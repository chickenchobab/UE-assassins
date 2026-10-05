// Fill out your copyright notice in the Description page of Project Settings.

#include "AssassinsMigrationLibrary.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/MemberReference.h"
#include "K2Node.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Event.h"
#include "K2Node_Knot.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/StringOutputDevice.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/CoreNetTypes.h"
#include "UObject/UnrealType.h"

namespace AssassinsMigration
{
	static const FName NAME_MemberReference(TEXT("MemberReference"));
	static const FName NAME_BPVariableDescription(TEXT("BPVariableDescription"));
	static const FName NAME_UberGraphFrame(TEXT("UberGraphFrame"));
	static const FName NAME_PointerToUberGraphFrame(TEXT("PointerToUberGraphFrame"));
	static const FName NAME_K2NodeKnot(TEXT("K2Node_Knot"));
	static const FName NAME_EdGraphNodeComment(TEXT("EdGraphNode_Comment"));
	static const FName NAME_K2NodeFunctionEntry(TEXT("K2Node_FunctionEntry"));
	static const FName NAME_EdGraphPinType(TEXT("EdGraphPinType"));

	//////////////////////////////////////////////////////////////////////////
	// JSON

	static FString ToJsonString(const TSharedRef<FJsonObject>& Object)
	{
		FString Out;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Object, Writer);
		return Out;
	}

	static FString MakeErrorJson(const FString& Error)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("error"), Error);
		return ToJsonString(Object);
	}

	static TArray<TSharedPtr<FJsonValue>> ToJsonArray(const TArray<FString>& Strings)
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		for (const FString& String : Strings)
		{
			Values.Add(MakeShared<FJsonValueString>(String));
		}
		return Values;
	}

	//////////////////////////////////////////////////////////////////////////
	// Reflection

	// ExportText_InContainer skips values identical to zero(0, false, None, empty), which can't be restored then.
	// ExportTextItem_Direct always writes the value.
	static FString ExportPropertyValue(const FProperty* Property, const void* Container, int32 Index = 0)
	{
		FString Value;
		Property->ExportTextItem_Direct(Value, Property->ContainerPtrToValuePtr<void>(Container, Index), nullptr, nullptr, PPF_None);

		// Empty containers export nothing, but only "()" imports back as empty.
		if (Value.IsEmpty() && (Property->IsA<FArrayProperty>() || Property->IsA<FSetProperty>() || Property->IsA<FMapProperty>()))
		{
			Value = TEXT("()");
		}
		return Value;
	}

	static FString GetPropertyCPPType(const FProperty* Property)
	{
		FString ExtendedType;
		const FString Type = Property->GetCPPType(&ExtendedType);
		return Type + ExtendedType;
	}

	static TArray<FString> DescribePropertyFlags(uint64 Flags)
	{
		struct FFlagName
		{
			EPropertyFlags Flag;
			const TCHAR* Name;
		};

		static const FFlagName FlagNames[] = {
			{ CPF_Edit, TEXT("Edit") },
			{ CPF_BlueprintVisible, TEXT("BlueprintVisible") },
			{ CPF_BlueprintReadOnly, TEXT("BlueprintReadOnly") },
			{ CPF_DisableEditOnInstance, TEXT("DisableEditOnInstance") },
			{ CPF_DisableEditOnTemplate, TEXT("DisableEditOnTemplate") },
			{ CPF_EditConst, TEXT("EditConst") },
			{ CPF_Net, TEXT("Replicated") },
			{ CPF_RepNotify, TEXT("RepNotify") },
			{ CPF_Transient, TEXT("Transient") },
			{ CPF_Config, TEXT("Config") },
			{ CPF_SaveGame, TEXT("SaveGame") },
			{ CPF_ExposeOnSpawn, TEXT("ExposeOnSpawn") },
			{ CPF_AdvancedDisplay, TEXT("AdvancedDisplay") },
			{ CPF_NativeAccessSpecifierProtected, TEXT("Protected") },
			{ CPF_NativeAccessSpecifierPrivate, TEXT("Private") }
		};

		TArray<FString> Names;
		for (const FFlagName& FlagName : FlagNames)
		{
			if (Flags & static_cast<uint64>(FlagName.Flag))
			{
				Names.Add(FlagName.Name);
			}
		}
		return Names;
	}

	static FString DescribeFunctionFlags(uint64 Flags)
	{
		struct FFlagName
		{
			EFunctionFlags Flag;
			const TCHAR* Name;
		};

		static const FFlagName FlagNames[] = {
			{ FUNC_BlueprintPure, TEXT("Pure") },
			{ FUNC_Const, TEXT("Const") },
			{ FUNC_Protected, TEXT("Protected") },
			{ FUNC_Private, TEXT("Private") },
			{ FUNC_NetServer, TEXT("Server") },
			{ FUNC_NetClient, TEXT("Client") },
			{ FUNC_NetMulticast, TEXT("Multicast") },
			{ FUNC_NetReliable, TEXT("Reliable") }
		};

		TArray<FString> Names;
		for (const FFlagName& FlagName : FlagNames)
		{
			if (Flags & static_cast<uint64>(FlagName.Flag))
			{
				Names.Add(FlagName.Name);
			}
		}
		return FString::Join(Names, TEXT(" "));
	}

	// C++ like signature of a function generated from a graph, e.g. "FRotator GetTargetLookAtRotation(AActor* Target) const".
	static FString DescribeSignature(const UFunction* Function)
	{
		FString ReturnType = TEXT("void");
		TArray<FString> Params;
		for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			FString Type = GetPropertyCPPType(*It);
			if (It->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnType = Type;
				continue;
			}

			if (It->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				Type = (It->HasAnyPropertyFlags(CPF_ConstParm) ? TEXT("const ") : TEXT("")) + Type + TEXT("& /*ref*/");
			}
			else if (It->HasAnyPropertyFlags(CPF_OutParm))
			{
				Type += TEXT("& /*out*/");
			}
			Params.Add(Type + TEXT(" ") + It->GetName());
		}

		const TCHAR* ConstSuffix = Function->HasAnyFunctionFlags(FUNC_Const) ? TEXT(" const") : TEXT("");
		return FString::Printf(TEXT("%s %s(%s)%s"), *ReturnType, *Function->GetName(), *FString::Join(Params, TEXT(", ")), ConstSuffix);
	}

	static const UFunction* FindGeneratedFunction(const UBlueprint* Blueprint, FName FunctionName)
	{
		if (Blueprint->GeneratedClass == nullptr || FunctionName.IsNone())
		{
			return nullptr;
		}
		return Blueprint->GeneratedClass->FindFunctionByName(FunctionName, EIncludeSuperFlag::ExcludeSuper);
	}

	static bool ShouldDumpProperty(const FProperty* Property, bool bOwnedByBlueprint)
	{
		if (Property->HasAnyPropertyFlags(CPF_Deprecated))
		{
			return false;
		}

		if (Property->IsA<FMulticastDelegateProperty>() || Property->IsA<FDelegateProperty>())
		{
			return false;
		}

		if (Property->GetFName() == NAME_UberGraphFrame)
		{
			return false;
		}

		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct && (StructProperty->Struct->GetFName() == NAME_PointerToUberGraphFrame))
			{
				return false;
			}
		}

		// Every blueprint variable matters, even the transient ones take their default from the variable description.
		if (bOwnedByBlueprint)
		{
			return true;
		}

		return Property->HasAnyPropertyFlags(CPF_Edit) && !Property->HasAnyPropertyFlags(CPF_Transient);
	}

	//////////////////////////////////////////////////////////////////////////
	// Graphs

	struct FGraphEntry
	{
		FString Kind;
		UEdGraph* Graph = nullptr;
	};

	static TArray<FGraphEntry> GatherGraphs(const UBlueprint* Blueprint)
	{
		TArray<FGraphEntry> Entries;

		auto AddGraphs = [&Entries](const FString& Kind, const TArray<TObjectPtr<UEdGraph>>& Graphs)
		{
			for (UEdGraph* Graph : Graphs)
			{
				if (Graph)
				{
					Entries.Add({ Kind, Graph });
				}
			}
		};

		AddGraphs(TEXT("Ubergraph"), Blueprint->UbergraphPages);
		AddGraphs(TEXT("Function"), Blueprint->FunctionGraphs);
		AddGraphs(TEXT("Macro"), Blueprint->MacroGraphs);
		AddGraphs(TEXT("DelegateSignature"), Blueprint->DelegateSignatureGraphs);
		for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
		{
			AddGraphs(FString::Printf(TEXT("Interface(%s)"), *GetNameSafe(Interface.Interface.Get())), Interface.Graphs);
		}

		return Entries;
	}

	static const UFunction* FindGraphFunction(const UBlueprint* Blueprint, const FGraphEntry& Entry)
	{
		if (Entry.Kind == TEXT("DelegateSignature"))
		{
			return FindGeneratedFunction(Blueprint, FName(*(Entry.Graph->GetName() + HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX)));
		}

		if ((Entry.Kind == TEXT("Function")) || Entry.Kind.StartsWith(TEXT("Interface")))
		{
			return FindGeneratedFunction(Blueprint, Entry.Graph->GetFName());
		}

		return nullptr;
	}

	static const UFunction* FindOverriddenFunction(const UBlueprint* Blueprint, const FGraphEntry& Entry)
	{
		if ((Entry.Kind != TEXT("Function")) || (Blueprint->ParentClass == nullptr))
		{
			return nullptr;
		}
		return Blueprint->ParentClass->FindFunctionByName(Entry.Graph->GetFName());
	}

	static bool IsKnot(const UEdGraphNode* Node)
	{
		return Node && (Node->GetClass()->GetFName() == NAME_K2NodeKnot);
	}

	// Nodes whose pins take the type of what they are connected to(reroutes, array nodes, macro instances).
	// Their pins still carry the previous type when a connection is remade, and the schema compares against it.
	static UK2Node* AsWildcardAdaptingNode(UEdGraphNode* Node)
	{
		static const TSet<FName> WildcardNodeClasses = {
			NAME_K2NodeKnot,
			FName(TEXT("K2Node_CallArrayFunction")),
			FName(TEXT("K2Node_MacroInstance")),
			FName(TEXT("K2Node_GetArrayItem")),
			FName(TEXT("K2Node_MakeArray")),
			FName(TEXT("K2Node_MakeSet")),
			FName(TEXT("K2Node_MakeMap")),
			FName(TEXT("K2Node_Select"))
		};

		return (Node && WildcardNodeClasses.Contains(Node->GetClass()->GetFName())) ? Cast<UK2Node>(Node) : nullptr;
	}

	static bool IsComment(const UEdGraphNode* Node)
	{
		return Node && (Node->GetClass()->GetFName() == NAME_EdGraphNodeComment);
	}

	static bool IsExecPin(const UEdGraphPin* Pin)
	{
		return Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
	}

	static bool HasExecPins(const UEdGraphNode* Node)
	{
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && IsExecPin(Pin))
			{
				return true;
			}
		}
		return false;
	}

	// Nodes where the execution starts in a graph(events, function entries, macro tunnels).
	static bool IsEntryNode(const UEdGraphNode* Node)
	{
		bool bHasExecInput = false;
		bool bHasExecOutput = false;
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && IsExecPin(Pin))
			{
				bHasExecInput |= (Pin->Direction == EGPD_Input);
				bHasExecOutput |= (Pin->Direction == EGPD_Output);
			}
		}
		return bHasExecOutput && !bHasExecInput;
	}

	// Follows the links through reroute nodes so that the listing only shows meaningful endpoints.
	static void ResolveLinks(const UEdGraphPin* Pin, TArray<const UEdGraphPin*>& OutPins, int32 Depth = 0)
	{
		if (Depth > 64)
		{
			return;
		}

		for (const UEdGraphPin* Linked : Pin->LinkedTo)
		{
			if (Linked == nullptr)
			{
				continue;
			}

			const UEdGraphNode* LinkedNode = Linked->GetOwningNode();
			if (IsKnot(LinkedNode))
			{
				for (const UEdGraphPin* KnotPin : LinkedNode->Pins)
				{
					if (KnotPin && (KnotPin->Direction != Linked->Direction))
					{
						ResolveLinks(KnotPin, OutPins, Depth + 1);
					}
				}
				continue;
			}

			OutPins.Add(Linked);
		}
	}

	static FString JoinPinLabels(const TArray<const UEdGraphPin*>& Pins)
	{
		TArray<FString> Labels;
		for (const UEdGraphPin* Pin : Pins)
		{
			Labels.Add(FString::Printf(TEXT("%s.%s"), *Pin->GetOwningNode()->GetName(), *Pin->PinName.ToString()));
		}
		return FString::Join(Labels, TEXT(", "));
	}

	static FString GetPinTypeText(const UEdGraphPin* Pin)
	{
		return UEdGraphSchema_K2::TypeToText(Pin->PinType).ToString();
	}

	static FString GetPinDefaultText(const UEdGraphPin* Pin)
	{
		if (Pin->DefaultObject)
		{
			return Pin->DefaultObject->GetPathName();
		}

		if (!Pin->DefaultTextValue.IsEmpty())
		{
			return FString::Printf(TEXT("\"%s\""), *Pin->DefaultTextValue.ToString());
		}

		return Pin->DefaultValue;
	}

	static FString SingleLine(const FString& Text)
	{
		return Text.Replace(TEXT("\r"), TEXT("")).Replace(TEXT("\n"), TEXT(" | "));
	}

	static FString DescribeMemberReference(const FMemberReference& Reference)
	{
		FString Owner;
		if (Reference.IsSelfContext())
		{
			Owner = TEXT("self");
		}
		else if (Reference.IsLocalScope())
		{
			Owner = TEXT("local");
		}
		else
		{
			Owner = GetNameSafe(Reference.GetMemberParentClass());
		}
		return FString::Printf(TEXT("%s::%s"), *Owner, *Reference.GetMemberName().ToString());
	}

	static const FMemberReference* FindMemberReference(const UEdGraphNode* Node, const TCHAR* PropertyName)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(Node->GetClass()->FindPropertyByName(PropertyName));
		if (StructProperty && StructProperty->Struct && (StructProperty->Struct->GetFName() == NAME_MemberReference))
		{
			return StructProperty->ContainerPtrToValuePtr<FMemberReference>(Node);
		}
		return nullptr;
	}

	// Name of the function an event node generates, and whether it is a custom event(not an override).
	static FName GetEventFunctionName(const UEdGraphNode* Node, bool& bOutIsCustom)
	{
		bOutIsCustom = false;

		const FMemberReference* EventReference = FindMemberReference(Node, TEXT("EventReference"));
		if (EventReference == nullptr)
		{
			return NAME_None;
		}

		if (const FBoolProperty* OverrideProperty = CastField<FBoolProperty>(Node->GetClass()->FindPropertyByName(TEXT("bOverrideFunction"))))
		{
			bOutIsCustom = !OverrideProperty->GetPropertyValue_InContainer(Node);
		}

		if (bOutIsCustom)
		{
			if (const FNameProperty* CustomNameProperty = CastField<FNameProperty>(Node->GetClass()->FindPropertyByName(TEXT("CustomFunctionName"))))
			{
				return CustomNameProperty->GetPropertyValue_InContainer(Node);
			}
		}

		return EventReference->GetMemberName();
	}

	static FString DescribeNodeDetails(const UEdGraphNode* Node)
	{
		static const TCHAR* DetailNames[] = {
			TEXT("FunctionReference"),
			TEXT("EventReference"),
			TEXT("VariableReference"),
			TEXT("DelegateReference"),
			TEXT("CustomFunctionName"),
			TEXT("SelectedFunctionName"),
			TEXT("ProxyFactoryClass"),
			TEXT("ProxyFactoryFunctionName"),
			TEXT("ProxyClass"),
			TEXT("TargetType"),
			TEXT("StructType"),
			TEXT("Enum"),
			TEXT("CustomClass"),
			TEXT("MacroGraphReference")
		};

		TArray<FString> Details;
		for (const TCHAR* DetailName : DetailNames)
		{
			const FProperty* Property = Node->GetClass()->FindPropertyByName(DetailName);
			if (Property == nullptr)
			{
				continue;
			}

			const void* Value = Property->ContainerPtrToValuePtr<void>(Node);
			FString Text;

			if (const FMemberReference* Reference = FindMemberReference(Node, DetailName))
			{
				if (Reference->GetMemberName().IsNone())
				{
					continue;
				}
				Text = DescribeMemberReference(*Reference);
			}
			else if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
			{
				const FName Name = NameProperty->GetPropertyValue(Value);
				if (Name.IsNone())
				{
					continue;
				}
				Text = Name.ToString();
			}
			else if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
			{
				const UObject* Object = ObjectProperty->GetObjectPropertyValue(Value);
				if (Object == nullptr)
				{
					continue;
				}
				Text = Object->GetName();
			}
			else
			{
				Property->ExportTextItem_Direct(Text, Value, nullptr, nullptr, PPF_None);
				if (Text.IsEmpty() || (Text == TEXT("()")))
				{
					continue;
				}
			}

			Details.Add(FString::Printf(TEXT("%s=%s"), DetailName, *Text));
		}

		// Function flags of events(FunctionFlags) and function entries(ExtraFlags).
		for (const TCHAR* FlagsName : { TEXT("FunctionFlags"), TEXT("ExtraFlags") })
		{
			if (const FNumericProperty* FlagsProperty = CastField<FNumericProperty>(Node->GetClass()->FindPropertyByName(FlagsName)))
			{
				const uint64 Flags = FlagsProperty->GetUnsignedIntPropertyValue(FlagsProperty->ContainerPtrToValuePtr<void>(Node));
				const FString FlagsText = DescribeFunctionFlags(Flags);
				if (!FlagsText.IsEmpty())
				{
					Details.Add(FString::Printf(TEXT("flags=%s"), *FlagsText));
				}
			}
		}

		return Details.IsEmpty() ? FString() : FString::Printf(TEXT("  {%s}"), *FString::Join(Details, TEXT(", ")));
	}

	/**
	 * FGraphListingWriter
	 *
	 * Writes a graph as a listing which reads like code:
	 * entry nodes first, then the execution flow depth first, with the pure nodes written right before their first consumer.
	 */
	class FGraphListingWriter
	{
	public:

		FGraphListingWriter(const UBlueprint* InBlueprint, FString& InOut)
			: Blueprint(InBlueprint)
			, Out(InOut)
		{
		}

		void WriteGraph(const FGraphEntry& Entry)
		{
			Visited.Reset();

			Out += TEXT("\n################################################################\n");
			Out += FString::Printf(TEXT("# %s: %s\n"), *Entry.Kind, *Entry.Graph->GetName());

			if (const UFunction* Function = FindGraphFunction(Blueprint, Entry))
			{
				const FString FlagsText = DescribeFunctionFlags(Function->FunctionFlags);
				Out += FString::Printf(TEXT("# signature: %s%s\n"), *DescribeSignature(Function), FlagsText.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("  [%s]"), *FlagsText));
			}

			if (const UFunction* Overridden = FindOverriddenFunction(Blueprint, Entry))
			{
				Out += FString::Printf(TEXT("# overrides: %s::%s\n"), *GetNameSafe(Overridden->GetOwnerClass()), *Overridden->GetName());
			}

			TArray<const UEdGraphNode*> Nodes;
			TArray<const UEdGraphNode*> Comments;
			for (const UEdGraphNode* Node : Entry.Graph->Nodes)
			{
				if (Node == nullptr || IsKnot(Node))
				{
					continue;
				}
				(IsComment(Node) ? Comments : Nodes).Add(Node);
			}

			WriteComments(Comments, Nodes);

			SortByPosition(Nodes);

			for (const UEdGraphNode* Node : Nodes)
			{
				if (IsEntryNode(Node))
				{
					VisitExecNode(Node);
				}
			}

			// Unreachable execution chains.
			for (const UEdGraphNode* Node : Nodes)
			{
				if (HasExecPins(Node))
				{
					VisitExecNode(Node);
				}
			}

			// Pure nodes nobody consumes.
			for (const UEdGraphNode* Node : Nodes)
			{
				VisitPureNode(Node);
			}
		}

	private:

		static void SortByPosition(TArray<const UEdGraphNode*>& Nodes)
		{
			Nodes.Sort([](const UEdGraphNode& A, const UEdGraphNode& B)
			{
				return (A.NodePosY != B.NodePosY) ? (A.NodePosY < B.NodePosY) : (A.NodePosX < B.NodePosX);
			});
		}

		void WriteComments(const TArray<const UEdGraphNode*>& Comments, const TArray<const UEdGraphNode*>& Nodes)
		{
			for (const UEdGraphNode* Comment : Comments)
			{
				TArray<FString> Contained;
				for (const UEdGraphNode* Node : Nodes)
				{
					const bool bInsideX = (Node->NodePosX >= Comment->NodePosX) && (Node->NodePosX <= Comment->NodePosX + Comment->NodeWidth);
					const bool bInsideY = (Node->NodePosY >= Comment->NodePosY) && (Node->NodePosY <= Comment->NodePosY + Comment->NodeHeight);
					if (bInsideX && bInsideY)
					{
						Contained.Add(Node->GetName());
					}
				}
				Out += FString::Printf(TEXT("# COMMENT \"%s\" contains: %s\n"), *SingleLine(Comment->NodeComment), *FString::Join(Contained, TEXT(", ")));
			}
		}

		void VisitExecNode(const UEdGraphNode* Node)
		{
			if (Visited.Contains(Node))
			{
				return;
			}

			VisitDataDependencies(Node, 0);
			Visited.Add(Node);
			WriteNode(Node);

			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && (Pin->Direction == EGPD_Output) && IsExecPin(Pin))
				{
					TArray<const UEdGraphPin*> Targets;
					ResolveLinks(Pin, Targets);
					for (const UEdGraphPin* Target : Targets)
					{
						VisitExecNode(Target->GetOwningNode());
					}
				}
			}
		}

		void VisitPureNode(const UEdGraphNode* Node)
		{
			if (Visited.Contains(Node))
			{
				return;
			}

			VisitDataDependencies(Node, 0);
			Visited.Add(Node);
			WriteNode(Node);
		}

		void VisitDataDependencies(const UEdGraphNode* Node, int32 Depth)
		{
			if (Depth > 64)
			{
				return;
			}

			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin == nullptr || (Pin->Direction != EGPD_Input) || IsExecPin(Pin))
				{
					continue;
				}

				TArray<const UEdGraphPin*> Sources;
				ResolveLinks(Pin, Sources);
				for (const UEdGraphPin* Source : Sources)
				{
					const UEdGraphNode* SourceNode = Source->GetOwningNode();
					if (!Visited.Contains(SourceNode) && !HasExecPins(SourceNode))
					{
						VisitDataDependencies(SourceNode, Depth + 1);
						if (!Visited.Contains(SourceNode))
						{
							Visited.Add(SourceNode);
							WriteNode(SourceNode);
						}
					}
				}
			}
		}

		void WriteNode(const UEdGraphNode* Node)
		{
			const bool bPure = !HasExecPins(Node);
			const bool bEntry = IsEntryNode(Node);

			Out += FString::Printf(TEXT("\n[%s] %s%s%s%s\n"),
				*Node->GetName(),
				*SingleLine(Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString()),
				bPure ? TEXT("  (pure)") : TEXT(""),
				Node->IsNodeEnabled() ? TEXT("") : TEXT("  (DISABLED)"),
				*DescribeNodeDetails(Node));

			if (!Node->NodeComment.IsEmpty())
			{
				Out += FString::Printf(TEXT("    // %s\n"), *SingleLine(Node->NodeComment));
			}

			bool bIsCustomEvent = false;
			const FName EventFunctionName = GetEventFunctionName(Node, bIsCustomEvent);
			if (const UFunction* EventFunction = FindGeneratedFunction(Blueprint, EventFunctionName))
			{
				Out += FString::Printf(TEXT("    signature: %s\n"), *DescribeSignature(EventFunction));
			}

			WriteLocalVariables(Node);

			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin == nullptr)
				{
					continue;
				}

				const bool bExec = IsExecPin(Pin);
				TArray<const UEdGraphPin*> Links;
				ResolveLinks(Pin, Links);

				if (Pin->Direction == EGPD_Input)
				{
					if (bExec)
					{
						continue;
					}

					if (Links.Num() > 0)
					{
						Out += FString::Printf(TEXT("    in  %s: %s <- %s\n"), *Pin->PinName.ToString(), *GetPinTypeText(Pin), *JoinPinLabels(Links));
						continue;
					}

					if (Pin->bHidden || (Pin->PinName == UEdGraphSchema_K2::PN_Self))
					{
						continue;
					}

					const FString Default = GetPinDefaultText(Pin);
					Out += FString::Printf(TEXT("    in  %s: %s%s\n"), *Pin->PinName.ToString(), *GetPinTypeText(Pin), Default.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" = %s"), *Default));
				}
				else if (bExec)
				{
					if (Links.Num() > 0)
					{
						Out += FString::Printf(TEXT("    -> %s: %s\n"), *Pin->PinName.ToString(), *JoinPinLabels(Links));
					}
				}
				else if (bEntry && !Pin->bHidden && (Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Delegate))
				{
					// Parameters of an entry point(the delegate pin every event node has is left out).
					Out += FString::Printf(TEXT("    out %s: %s\n"), *Pin->PinName.ToString(), *GetPinTypeText(Pin));
				}
			}
		}

		void WriteLocalVariables(const UEdGraphNode* Node)
		{
			const FArrayProperty* LocalsProperty = CastField<FArrayProperty>(Node->GetClass()->FindPropertyByName(TEXT("LocalVariables")));
			if (LocalsProperty == nullptr)
			{
				return;
			}

			const FStructProperty* InnerProperty = CastField<FStructProperty>(LocalsProperty->Inner);
			if (InnerProperty == nullptr || InnerProperty->Struct == nullptr || (InnerProperty->Struct->GetFName() != NAME_BPVariableDescription))
			{
				return;
			}

			FScriptArrayHelper Helper(LocalsProperty, LocalsProperty->ContainerPtrToValuePtr<void>(Node));
			for (int32 Index = 0; Index < Helper.Num(); ++Index)
			{
				const FBPVariableDescription* Variable = reinterpret_cast<const FBPVariableDescription*>(Helper.GetRawPtr(Index));
				Out += FString::Printf(TEXT("    local %s: %s%s\n"),
					*Variable->VarName.ToString(),
					*UEdGraphSchema_K2::TypeToText(Variable->VarType).ToString(),
					Variable->DefaultValue.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" = %s"), *Variable->DefaultValue));
			}
		}

	private:

		const UBlueprint* Blueprint;
		FString& Out;
		TSet<const UEdGraphNode*> Visited;
	};

	//////////////////////////////////////////////////////////////////////////
	// Compile

	static FString GetBlueprintStatusText(EBlueprintStatus Status)
	{
		switch (Status)
		{
		case BS_Dirty: return TEXT("Dirty");
		case BS_Error: return TEXT("Error");
		case BS_UpToDate: return TEXT("UpToDate");
		case BS_BeingCreated: return TEXT("BeingCreated");
		case BS_UpToDateWithWarnings: return TEXT("UpToDateWithWarnings");
		default: return TEXT("Unknown");
		}
	}

	static TSharedRef<FJsonObject> MakeCompileResultsJson(const UBlueprint* Blueprint, const FCompilerResultsLog& Results)
	{
		TArray<FString> Messages;
		for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages)
		{
			const EMessageSeverity::Type Severity = Message->GetSeverity();
			if (Severity == EMessageSeverity::Error)
			{
				Messages.Add(TEXT("Error: ") + Message->ToText().ToString());
			}
			else if ((Severity == EMessageSeverity::Warning) || (Severity == EMessageSeverity::PerformanceWarning))
			{
				Messages.Add(TEXT("Warning: ") + Message->ToText().ToString());
			}
		}

		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
		Object->SetStringField(TEXT("parent_class"), GetPathNameSafe(Blueprint->ParentClass));
		Object->SetStringField(TEXT("status"), GetBlueprintStatusText(Blueprint->Status));
		Object->SetNumberField(TEXT("num_errors"), Results.NumErrors);
		Object->SetNumberField(TEXT("num_warnings"), Results.NumWarnings);
		Object->SetArrayField(TEXT("messages"), ToJsonArray(Messages));
		return Object;
	}

	//////////////////////////////////////////////////////////////////////////
	// Dump

	static TSharedRef<FJsonObject> DumpProperties(const UClass* Class, const UObject* Object)
	{
		const TSharedRef<FJsonObject> Properties = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> It(Class, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			const FProperty* Property = *It;
			const UClass* OwnerClass = Property->GetOwnerClass();
			const bool bOwnedByBlueprint = OwnerClass && !OwnerClass->HasAnyClassFlags(CLASS_Native);
			if (!ShouldDumpProperty(Property, bOwnedByBlueprint))
			{
				continue;
			}

			for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
			{
				const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
				Entry->SetStringField(TEXT("value"), ExportPropertyValue(Property, Object, Index));
				Entry->SetStringField(TEXT("type"), GetPropertyCPPType(Property));
				Entry->SetStringField(TEXT("owner"), GetNameSafe(OwnerClass));
				Entry->SetBoolField(TEXT("native"), !bOwnedByBlueprint);
				if (Property->HasAnyPropertyFlags(CPF_EditConst))
				{
					Entry->SetBoolField(TEXT("edit_const"), true);
				}
				if (Property->HasAnyPropertyFlags(CPF_InstancedReference | CPF_ContainsInstancedReference))
				{
					Entry->SetBoolField(TEXT("instanced"), true);
				}

				const FString Key = (Property->ArrayDim == 1) ? Property->GetName() : FString::Printf(TEXT("%s[%d]"), *Property->GetName(), Index);
				Properties->SetObjectField(Key, Entry);
			}
		}
		return Properties;
	}

	//////////////////////////////////////////////////////////////////////////
	// Events

	// A custom event can't share its name with a native function(UK2Node_CustomEvent reports "name conflicts with a native function"),
	// so the custom events a native parent now declares as blueprint events become overrides of them, with the same links.
	static void ConvertCustomEventsToOverrides(UBlueprint* Blueprint, TArray<FString>& OutConverted, TArray<FString>& OutProblems)
	{
		if (Blueprint->ParentClass == nullptr)
		{
			return;
		}

		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			if (Graph == nullptr)
			{
				continue;
			}

			TArray<UK2Node_CustomEvent*> CustomEvents;
			Graph->GetNodesOfClass(CustomEvents);

			for (UK2Node_CustomEvent* CustomEvent : CustomEvents)
			{
				const FName EventName = CustomEvent->CustomFunctionName;
				const UFunction* ParentFunction = Blueprint->ParentClass->FindFunctionByName(EventName);
				if (ParentFunction == nullptr || !ParentFunction->HasAnyFunctionFlags(FUNC_BlueprintEvent))
				{
					continue;
				}

				// Only a native declaration clashes, overriding a custom event of a blueprint parent is fine.
				const UFunction* DeclaredFunction = ParentFunction;
				while (DeclaredFunction->GetSuperFunction())
				{
					DeclaredFunction = DeclaredFunction->GetSuperFunction();
				}

				UClass* DeclaringClass = DeclaredFunction->GetOwnerClass();
				if (DeclaringClass == nullptr || !DeclaringClass->HasAnyClassFlags(CLASS_Native))
				{
					continue;
				}

				UK2Node_Event* EventNode = NewObject<UK2Node_Event>(Graph);
				EventNode->EventReference.SetExternalMember(EventName, DeclaringClass);
				EventNode->bOverrideFunction = true;
				EventNode->NodePosX = CustomEvent->NodePosX;
				EventNode->NodePosY = CustomEvent->NodePosY;
				EventNode->NodeComment = CustomEvent->NodeComment;
				EventNode->bCommentBubbleVisible = CustomEvent->bCommentBubbleVisible;
				EventNode->SetFlags(RF_Transactional);
				EventNode->CreateNewGuid();
				Graph->AddNode(EventNode, /*bFromUI*/ false, /*bSelectNewNode*/ false);
				EventNode->PostPlacedNewNode();
				EventNode->AllocateDefaultPins();

				for (UEdGraphPin* OldPin : CustomEvent->Pins)
				{
					if (OldPin == nullptr || OldPin->LinkedTo.Num() == 0)
					{
						continue;
					}

					UEdGraphPin* NewPin = EventNode->FindPin(OldPin->PinName, OldPin->Direction);
					if (NewPin == nullptr)
					{
						OutProblems.Add(FString::Printf(TEXT("%s: the override has no pin '%s', its links were dropped"), *EventName.ToString(), *OldPin->PinName.ToString()));
						continue;
					}

					const TArray<UEdGraphPin*> LinkedPins = OldPin->LinkedTo;
					for (UEdGraphPin* LinkedPin : LinkedPins)
					{
						OldPin->BreakLinkTo(LinkedPin);
						NewPin->MakeLinkTo(LinkedPin);
					}
				}

				FBlueprintEditorUtils::RemoveNode(Blueprint, CustomEvent, /*bDontRecompile*/ true);
				OutConverted.Add(FString::Printf(TEXT("%s -> %s::%s"), *EventName.ToString(), *DeclaringClass->GetName(), *EventName.ToString()));
			}
		}
	}

	struct FPinFixReport
	{
		TArray<FString> MovedLinks;
		TArray<FString> RefusedLinks;
		TArray<FString> UnmatchedPins;
		int32 NumRemovedPins = 0;

		int32 NumChanges() const { return MovedLinks.Num() + NumRemovedPins; }
	};

	// One pass over the graphs: the links of every orphaned pin move to the live pin of the same name on the same node.
	static void FixOrphanedPinsOnce(UBlueprint* Blueprint, FPinFixReport& Report)
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
		{
			for (UEdGraphNode* Node : Entry.Graph->Nodes)
			{
				if (Node == nullptr)
				{
					continue;
				}

				TArray<UEdGraphPin*> Orphans;
				for (UEdGraphPin* Pin : Node->Pins)
				{
					if (Pin && Pin->bOrphanedPin)
					{
						Orphans.Add(Pin);
					}
				}

				for (UEdGraphPin* Orphan : Orphans)
				{
					UEdGraphPin* LivePin = nullptr;
					for (UEdGraphPin* Pin : Node->Pins)
					{
						if (Pin && !Pin->bOrphanedPin && (Pin->PinName == Orphan->PinName) && (Pin->Direction == Orphan->Direction))
						{
							LivePin = Pin;
							break;
						}
					}

					const FString OrphanLabel = FString::Printf(TEXT("%s.%s"), *Node->GetName(), *Orphan->PinName.ToString());
					if (LivePin == nullptr)
					{
						Report.UnmatchedPins.AddUnique(OrphanLabel);
						continue;
					}

					const TArray<UEdGraphPin*> LinkedPins = Orphan->LinkedTo;
					for (UEdGraphPin* LinkedPin : LinkedPins)
					{
						const FString LinkLabel = FString::Printf(TEXT("%s -> %s.%s"), *OrphanLabel, *LinkedPin->GetOwningNode()->GetName(), *LinkedPin->PinName.ToString());

						Orphan->BreakLinkTo(LinkedPin);
						if (Schema->TryCreateConnection(LivePin, LinkedPin))
						{
							Report.MovedLinks.Add(LinkLabel);
							continue;
						}

						UK2Node* WildcardNode = AsWildcardAdaptingNode(LinkedPin->GetOwningNode());
						if (WildcardNode == nullptr)
						{
							// Keep the link where it was, so that nothing is lost.
							Orphan->MakeLinkTo(LinkedPin);
							Report.RefusedLinks.Add(LinkLabel);
							continue;
						}

						// Remember what it was connected to: rebuilding it drops the links it no longer accepts.
						TMap<FName, TArray<UEdGraphPin*>> PreviousLinks;
						for (UEdGraphPin* NodePin : WildcardNode->Pins)
						{
							if (NodePin)
							{
								PreviousLinks.Add(NodePin->PinName, NodePin->LinkedTo);
							}
						}

						// Hand the node the new type: the pin it is linked through, the pins that shared the previous type
						// (the element of an array, the other side of a reroute), and the type it builds its expansion from.
						WildcardNode->Modify();

						const FEdGraphPinType PreviousType = LinkedPin->PinType;
						LinkedPin->PinType = LivePin->PinType;

						for (UEdGraphPin* NodePin : WildcardNode->Pins)
						{
							if ((NodePin == nullptr) || (NodePin == LinkedPin))
							{
								continue;
							}

							if ((NodePin->PinType.PinCategory == PreviousType.PinCategory) && (NodePin->PinType.PinSubCategoryObject == PreviousType.PinSubCategoryObject))
							{
								// The container stays as it is: an array pin and its element pin differ by that alone.
								NodePin->PinType.PinCategory = LivePin->PinType.PinCategory;
								NodePin->PinType.PinSubCategory = LivePin->PinType.PinSubCategory;
								NodePin->PinType.PinSubCategoryObject = LivePin->PinType.PinSubCategoryObject;
							}
						}

						const FStructProperty* ResolvedTypeProperty = CastField<FStructProperty>(WildcardNode->GetClass()->FindPropertyByName(TEXT("ResolvedWildcardType")));
						if (ResolvedTypeProperty && ResolvedTypeProperty->Struct && (ResolvedTypeProperty->Struct->GetFName() == NAME_EdGraphPinType))
						{
							*ResolvedTypeProperty->ContainerPtrToValuePtr<FEdGraphPinType>(WildcardNode) = LivePin->PinType;
						}

						LivePin->MakeLinkTo(LinkedPin);
						WildcardNode->NotifyPinConnectionListChanged(LinkedPin);
						Report.MovedLinks.Add(LinkLabel + TEXT(" (wildcard retyped)"));

						for (const TPair<FName, TArray<UEdGraphPin*>>& PreviousPinLinks : PreviousLinks)
						{
							UEdGraphPin* NodePin = WildcardNode->FindPin(PreviousPinLinks.Key);
							if (NodePin == nullptr)
							{
								continue;
							}

							for (UEdGraphPin* PreviousLink : PreviousPinLinks.Value)
							{
								if ((PreviousLink == nullptr) || NodePin->LinkedTo.Contains(PreviousLink))
								{
									continue;
								}

								const FString RestoredLabel = FString::Printf(TEXT("%s.%s -> %s.%s"), *WildcardNode->GetName(), *NodePin->PinName.ToString(),
									*PreviousLink->GetOwningNode()->GetName(), *PreviousLink->PinName.ToString());
								if (Schema->TryCreateConnection(NodePin, PreviousLink))
								{
									Report.MovedLinks.Add(RestoredLabel + TEXT(" (restored)"));
								}
								else
								{
									Report.RefusedLinks.Add(RestoredLabel + TEXT(" (lost by the rebuild)"));
								}
							}
						}
					}

					if (Orphan->LinkedTo.Num() == 0)
					{
						Node->RemovePin(Orphan);
						++Report.NumRemovedPins;
					}
				}
			}
		}
	}

	static const UFunction* FindFirstDeclaration(const UFunction* Function)
	{
		while (Function && Function->GetSuperFunction())
		{
			Function = Function->GetSuperFunction();
		}
		return Function;
	}

	// After the parent changed, or a native event moved to another native class:
	// - override events point to the class declaring the function now,
	// - "Parent: X" calls point to the new parent, and are removed(their execution links bridged) when the parent's
	//   function is a native event without any implementation, calling it does nothing.
	static void FixupInheritedReferences(UBlueprint* Blueprint, TArray<FString>& OutFixed, TArray<FString>& OutProblems)
	{
		if (Blueprint->ParentClass == nullptr)
		{
			return;
		}

		for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
		{
			TArray<UK2Node_Event*> Events;
			Entry.Graph->GetNodesOfClass(Events);
			for (UK2Node_Event* Event : Events)
			{
				if (!Event->bOverrideFunction || Event->IsA<UK2Node_CustomEvent>())
				{
					continue;
				}

				// Interface events come from the interface, not from the parent.
				const UClass* EventOwner = Event->EventReference.GetMemberParentClass();
				if (EventOwner && EventOwner->HasAnyClassFlags(CLASS_Interface))
				{
					continue;
				}

				const FName Name = Event->EventReference.GetMemberName();
				const UFunction* Declaration = FindFirstDeclaration(Blueprint->ParentClass->FindFunctionByName(Name));
				if (Declaration == nullptr)
				{
					OutProblems.Add(FString::Printf(TEXT("%s: no function %s in the parent anymore"), *Event->GetName(), *Name.ToString()));
					continue;
				}

				UClass* DeclaringClass = Declaration->GetOwnerClass();
				if (Event->EventReference.GetMemberParentClass() != DeclaringClass)
				{
					Event->Modify();
					Event->EventReference.SetExternalMember(Name, DeclaringClass);
					OutFixed.Add(FString::Printf(TEXT("event %s -> %s::%s"), *Event->GetName(), *GetNameSafe(DeclaringClass), *Name.ToString()));
				}
			}

			TArray<UK2Node_CallParentFunction*> ParentCalls;
			Entry.Graph->GetNodesOfClass(ParentCalls);
			for (UK2Node_CallParentFunction* ParentCall : ParentCalls)
			{
				const FName Name = ParentCall->FunctionReference.GetMemberName();
				const UClass* Owner = ParentCall->FunctionReference.GetMemberParentClass();
				if (Owner && Blueprint->ParentClass->IsChildOf(Owner))
				{
					continue;
				}

				const UFunction* ParentFunction = Blueprint->ParentClass->FindFunctionByName(Name);
				if (ParentFunction == nullptr)
				{
					OutProblems.Add(FString::Printf(TEXT("%s: no function %s in the parent anymore"), *ParentCall->GetName(), *Name.ToString()));
					continue;
				}

				const bool bNothingToCall = ParentFunction->HasAnyFunctionFlags(FUNC_BlueprintEvent) && !ParentFunction->HasAnyFunctionFlags(FUNC_Native)
					&& ParentFunction->GetOwnerClass()->HasAnyClassFlags(CLASS_Native);
				if (bNothingToCall && !ParentCall->IsNodePure())
				{
					UEdGraphPin* ExecPin = ParentCall->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input);
					UEdGraphPin* ThenPin = ParentCall->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
					if (ExecPin && ThenPin)
					{
						for (UEdGraphPin* Source : ExecPin->LinkedTo)
						{
							for (UEdGraphPin* Target : ThenPin->LinkedTo)
							{
								Source->MakeLinkTo(Target);
							}
						}
					}

					OutFixed.Add(FString::Printf(TEXT("removed %s, %s::%s has no implementation"), *ParentCall->GetName(), *GetNameSafe(ParentFunction->GetOwnerClass()), *Name.ToString()));
					FBlueprintEditorUtils::RemoveNode(Blueprint, ParentCall, /*bDontRecompile*/ true);
					continue;
				}

				ParentCall->Modify();
				ParentCall->FunctionReference.SetExternalMember(Name, ParentFunction->GetOwnerClass());
				OutFixed.Add(FString::Printf(TEXT("parent call %s -> %s::%s"), *ParentCall->GetName(), *GetNameSafe(ParentFunction->GetOwnerClass()), *Name.ToString()));
			}
		}
	}
}

TArray<FString> UAssassinsMigrationLibrary::FindBlueprintsDerivedFrom(const FString& BaseClassPath)
{
	TArray<FString> BlueprintPaths;

	const FTopLevelAssetPath BaseClass(BaseClassPath);
	if (!BaseClass.IsValid())
	{
		return BlueprintPaths;
	}

	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	AssetRegistry.WaitForCompletion();

	TSet<FTopLevelAssetPath> DerivedClassPaths;
	AssetRegistry.GetDerivedClassNames({ BaseClass }, {}, DerivedClassPaths);

	for (const FTopLevelAssetPath& ClassPath : DerivedClassPaths)
	{
		const FString PackageName = ClassPath.GetPackageName().ToString();
		if (PackageName.StartsWith(TEXT("/Script/")))
		{
			continue;
		}

		// "/Game/X/GA_Y.GA_Y_C" -> "/Game/X/GA_Y.GA_Y"
		FString AssetName = ClassPath.GetAssetName().ToString();
		AssetName.RemoveFromEnd(TEXT("_C"));

		// Loaded blueprints also bring their SKEL_/REINST_ classes, which aren't assets.
		if (AssetName != FPackageName::GetShortName(PackageName))
		{
			continue;
		}

		BlueprintPaths.Add(PackageName + TEXT(".") + AssetName);
	}

	BlueprintPaths.Sort();
	return BlueprintPaths;
}

TArray<FString> UAssassinsMigrationLibrary::GetReferencers(const FString& PackageName)
{
	TArray<FName> Referencers;
	IAssetRegistry::GetChecked().GetReferencers(FName(*PackageName), Referencers, UE::AssetRegistry::EDependencyCategory::Package);

	TArray<FString> Result;
	for (const FName& Referencer : Referencers)
	{
		const FString ReferencerName = Referencer.ToString();
		if (ReferencerName != PackageName)
		{
			Result.Add(ReferencerName);
		}
	}

	Result.Sort();
	return Result;
}

FString UAssassinsMigrationLibrary::DescribeBlueprint(UBlueprint* Blueprint)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("name"), Blueprint->GetName());
	Root->SetStringField(TEXT("path"), Blueprint->GetPathName());
	Root->SetStringField(TEXT("generated_class"), GetPathNameSafe(Blueprint->GeneratedClass));
	Root->SetStringField(TEXT("parent_class"), GetPathNameSafe(Blueprint->ParentClass));
	Root->SetStringField(TEXT("status"), GetBlueprintStatusText(Blueprint->Status));
	Root->SetBoolField(TEXT("is_data_only"), FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint));

	TArray<FString> Ancestry;
	FString NativeParentClass;
	for (const UClass* Class = Blueprint->ParentClass; Class; Class = Class->GetSuperClass())
	{
		Ancestry.Add(Class->GetPathName());
		if (NativeParentClass.IsEmpty() && Class->HasAnyClassFlags(CLASS_Native))
		{
			NativeParentClass = Class->GetPathName();
		}
	}
	Root->SetStringField(TEXT("native_parent_class"), NativeParentClass);
	Root->SetArrayField(TEXT("ancestry"), ToJsonArray(Ancestry));

	TArray<FString> Interfaces;
	for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
	{
		Interfaces.Add(GetPathNameSafe(Interface.Interface.Get()));
	}
	Root->SetArrayField(TEXT("interfaces"), ToJsonArray(Interfaces));

	TArray<FString> Variables;
	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		Variables.Add(Variable.VarName.ToString());
	}
	Root->SetArrayField(TEXT("variables"), ToJsonArray(Variables));

	TArray<TSharedPtr<FJsonValue>> Graphs;
	TArray<TSharedPtr<FJsonValue>> EntryPoints;
	for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
	{
		const TSharedRef<FJsonObject> GraphObject = MakeShared<FJsonObject>();
		GraphObject->SetStringField(TEXT("kind"), Entry.Kind);
		GraphObject->SetStringField(TEXT("name"), Entry.Graph->GetName());
		GraphObject->SetNumberField(TEXT("nodes"), Entry.Graph->Nodes.Num());
		if (const UFunction* Function = FindGraphFunction(Blueprint, Entry))
		{
			GraphObject->SetStringField(TEXT("signature"), DescribeSignature(Function));
		}
		if (const UFunction* Overridden = FindOverriddenFunction(Blueprint, Entry))
		{
			GraphObject->SetStringField(TEXT("overrides"), GetNameSafe(Overridden->GetOwnerClass()) + TEXT("::") + Overridden->GetName());
		}
		Graphs.Add(MakeShared<FJsonValueObject>(GraphObject));

		if (Entry.Kind != TEXT("Ubergraph"))
		{
			continue;
		}

		for (const UEdGraphNode* Node : Entry.Graph->Nodes)
		{
			bool bIsCustomEvent = false;
			const FName EventFunctionName = Node ? GetEventFunctionName(Node, bIsCustomEvent) : NAME_None;
			if (EventFunctionName.IsNone())
			{
				continue;
			}

			const TSharedRef<FJsonObject> EntryPoint = MakeShared<FJsonObject>();
			EntryPoint->SetStringField(TEXT("graph"), Entry.Graph->GetName());
			EntryPoint->SetStringField(TEXT("node"), Node->GetName());
			EntryPoint->SetStringField(TEXT("name"), EventFunctionName.ToString());
			EntryPoint->SetStringField(TEXT("kind"), bIsCustomEvent ? TEXT("custom") : TEXT("override"));
			EntryPoint->SetBoolField(TEXT("enabled"), Node->IsNodeEnabled());
			if (const UFunction* Function = FindGeneratedFunction(Blueprint, EventFunctionName))
			{
				EntryPoint->SetStringField(TEXT("signature"), DescribeSignature(Function));
				EntryPoint->SetStringField(TEXT("flags"), DescribeFunctionFlags(Function->FunctionFlags));
			}
			EntryPoints.Add(MakeShared<FJsonValueObject>(EntryPoint));
		}
	}
	Root->SetArrayField(TEXT("graphs"), Graphs);
	Root->SetArrayField(TEXT("entry_points"), EntryPoints);

	return ToJsonString(Root);
}

FString UAssassinsMigrationLibrary::ExportBlueprintVariables(UBlueprint* Blueprint)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	const UClass* GeneratedClass = Blueprint->GeneratedClass;
	const UObject* CDO = GeneratedClass ? GeneratedClass->GetDefaultObject() : nullptr;

	TArray<TSharedPtr<FJsonValue>> Variables;
	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("name"), Variable.VarName.ToString());
		Object->SetStringField(TEXT("friendly_name"), Variable.FriendlyName);
		Object->SetStringField(TEXT("category"), Variable.Category.ToString());
		Object->SetStringField(TEXT("type"), UEdGraphSchema_K2::TypeToText(Variable.VarType).ToString());
		Object->SetStringField(TEXT("pin_category"), Variable.VarType.PinCategory.ToString());
		Object->SetStringField(TEXT("pin_subcategory"), Variable.VarType.PinSubCategory.ToString());
		Object->SetStringField(TEXT("pin_subcategory_object"), GetPathNameSafe(Variable.VarType.PinSubCategoryObject.Get()));
		Object->SetArrayField(TEXT("flags"), ToJsonArray(DescribePropertyFlags(Variable.PropertyFlags)));
		Object->SetStringField(TEXT("rep_notify"), Variable.RepNotifyFunc.IsNone() ? FString() : Variable.RepNotifyFunc.ToString());
		Object->SetStringField(TEXT("replication_condition"), StaticEnum<ELifetimeCondition>()->GetNameStringByValue(static_cast<int64>(Variable.ReplicationCondition.GetValue())));
		Object->SetStringField(TEXT("default_value_description"), Variable.DefaultValue);

		const TSharedRef<FJsonObject> MetaData = MakeShared<FJsonObject>();
		for (const FBPVariableMetaDataEntry& Entry : Variable.MetaDataArray)
		{
			MetaData->SetStringField(Entry.DataKey.ToString(), Entry.DataValue);
		}
		Object->SetObjectField(TEXT("metadata"), MetaData);

		if (const FProperty* Property = GeneratedClass ? GeneratedClass->FindPropertyByName(Variable.VarName) : nullptr)
		{
			Object->SetStringField(TEXT("cpp_type"), GetPropertyCPPType(Property));
			Object->SetStringField(TEXT("default_value"), ExportPropertyValue(Property, CDO));
		}

		Variables.Add(MakeShared<FJsonValueObject>(Object));
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Root->SetArrayField(TEXT("variables"), Variables);
	return ToJsonString(Root);
}

FString UAssassinsMigrationLibrary::ExportBlueprintGraphsAsText(UBlueprint* Blueprint)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return FString();
	}

	FString Out;
	for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
	{
		TSet<UObject*> Nodes;
		for (UEdGraphNode* Node : Entry.Graph->Nodes)
		{
			if (Node)
			{
				Nodes.Add(Node);
			}
		}

		Out += FString::Printf(TEXT("// ==== %s: %s (%d nodes)\n"), *Entry.Kind, *Entry.Graph->GetName(), Nodes.Num());
		if (Nodes.Num() > 0)
		{
			FString Text;
			FEdGraphUtilities::ExportNodesToText(Nodes, Text);
			Out += Text;
		}
		Out += TEXT("\n");
	}
	return Out;
}

FString UAssassinsMigrationLibrary::ExportBlueprintGraphsAsListing(UBlueprint* Blueprint)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return FString();
	}

	FString Out = FString::Printf(TEXT("# Blueprint: %s\n# Parent: %s\n"), *Blueprint->GetPathName(), *GetPathNameSafe(Blueprint->ParentClass));

	FGraphListingWriter Writer(Blueprint, Out);
	for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
	{
		Writer.WriteGraph(Entry);
	}
	return Out;
}

FString UAssassinsMigrationLibrary::DumpClassDefaults(const FString& ClassPath)
{
	using namespace AssassinsMigration;

	const UClass* Class = LoadClass<UObject>(nullptr, *ClassPath);
	if (Class == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("Failed to load class '%s'."), *ClassPath));
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("class"), Class->GetPathName());
	Root->SetStringField(TEXT("parent_class"), GetPathNameSafe(Class->GetSuperClass()));
	Root->SetObjectField(TEXT("properties"), DumpProperties(Class, Class->GetDefaultObject()));
	return ToJsonString(Root);
}

FString UAssassinsMigrationLibrary::DumpObjectProperties(UObject* Object)
{
	using namespace AssassinsMigration;

	if (Object == nullptr)
	{
		return MakeErrorJson(TEXT("Object is null."));
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("object"), Object->GetPathName());
	Root->SetStringField(TEXT("class"), Object->GetClass()->GetPathName());
	Root->SetObjectField(TEXT("properties"), DumpProperties(Object->GetClass(), Object));
	return ToJsonString(Root);
}

FString UAssassinsMigrationLibrary::RebaseBlueprint(UBlueprint* Blueprint, const FString& NewParentClassPath, const TArray<FString>& GraphsToRemove)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	UClass* NewParentClass = nullptr;
	if (!NewParentClassPath.IsEmpty())
	{
		NewParentClass = LoadClass<UObject>(nullptr, *NewParentClassPath);
		if (NewParentClass == nullptr)
		{
			return MakeErrorJson(FString::Printf(TEXT("Failed to load class '%s'."), *NewParentClassPath));
		}
	}

	Blueprint->Modify();

	const FString OldParentClass = GetPathNameSafe(Blueprint->ParentClass);

	// Graphs first, e.g. a function the native parent now implements would clash with it.
	TArray<FString> RemovedGraphs;
	TArray<FString> MissingGraphs;
	for (const FString& GraphName : GraphsToRemove)
	{
		UEdGraph* GraphToRemove = nullptr;
		for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
		{
			if (Entry.Graph->GetName() == GraphName)
			{
				GraphToRemove = Entry.Graph;
				break;
			}
		}

		if (GraphToRemove == nullptr)
		{
			MissingGraphs.Add(GraphName);
			continue;
		}

		FBlueprintEditorUtils::RemoveGraph(Blueprint, GraphToRemove, EGraphRemoveFlags::MarkTransient);
		RemovedGraphs.Add(GraphName);
	}

	if (NewParentClass)
	{
		Blueprint->ParentClass = NewParentClass;
	}

	// Qualified: inside the class, the member function of the same name would hide it.
	TArray<FString> ConvertedEvents;
	TArray<FString> Problems;
	AssassinsMigration::ConvertCustomEventsToOverrides(Blueprint, ConvertedEvents, Problems);

	TArray<FString> FixedReferences;
	FixupInheritedReferences(Blueprint, FixedReferences, Problems);

	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint,
		EBlueprintCompileOptions::SkipSave | EBlueprintCompileOptions::UseDeltaSerializationDuringReinstancing | EBlueprintCompileOptions::SkipNewVariableDefaultsDetection,
		&Results);

	TArray<FString> RemainingVariables;
	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		RemainingVariables.Add(Variable.VarName.ToString());
	}

	const TSharedRef<FJsonObject> Report = MakeCompileResultsJson(Blueprint, Results);
	Report->SetStringField(TEXT("old_parent_class"), OldParentClass);
	Report->SetArrayField(TEXT("removed_graphs"), ToJsonArray(RemovedGraphs));
	Report->SetArrayField(TEXT("missing_graphs"), ToJsonArray(MissingGraphs));
	Report->SetArrayField(TEXT("converted_events"), ToJsonArray(ConvertedEvents));
	Report->SetArrayField(TEXT("fixed_references"), ToJsonArray(FixedReferences));
	Report->SetArrayField(TEXT("problems"), ToJsonArray(Problems));
	Report->SetArrayField(TEXT("remaining_variables"), ToJsonArray(RemainingVariables));
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::SetObjectProperty(UObject* Object, const FString& PropertyName, const FString& Value)
{
	using namespace AssassinsMigration;

	if (Object == nullptr)
	{
		return MakeErrorJson(TEXT("Object is null."));
	}

	FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
	if (Property == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("%s has no property %s."), *Object->GetClass()->GetName(), *PropertyName));
	}

	const FString Before = ExportPropertyValue(Property, Object);

	Object->Modify();
	Object->PreEditChange(Property);

	FStringOutputDevice Errors;
	const TCHAR* Result = Property->ImportText_Direct(*Value, Property->ContainerPtrToValuePtr<void>(Object), Object, PPF_None, &Errors);

	FPropertyChangedEvent ChangedEvent(Property, EPropertyChangeType::ValueSet);
	Object->PostEditChangeProperty(ChangedEvent);
	Object->MarkPackageDirty();

	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("object"), Object->GetPathName());
	Report->SetStringField(TEXT("property"), PropertyName);
	Report->SetStringField(TEXT("before"), Before);
	Report->SetStringField(TEXT("after"), ExportPropertyValue(Property, Object));
	if (Result == nullptr)
	{
		Report->SetStringField(TEXT("error"), Errors.IsEmpty() ? TEXT("ImportText failed.") : FString(Errors));
	}
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::ReplacePinDefaultObject(UBlueprint* Blueprint, const FString& OldObjectPath, const FString& NewObjectPath)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	UObject* NewObject = LoadObject<UObject>(nullptr, *NewObjectPath);
	if (NewObject == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("Failed to load '%s'."), *NewObjectPath));
	}

	Blueprint->Modify();

	TArray<FString> ReplacedPins;
	for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
	{
		for (UEdGraphNode* Node : Entry.Graph->Nodes)
		{
			if (Node == nullptr)
			{
				continue;
			}

			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->DefaultObject && (Pin->DefaultObject->GetPathName() == OldObjectPath))
				{
					Node->Modify();
					Pin->DefaultObject = NewObject;
					Node->PinDefaultValueChanged(Pin);
					ReplacedPins.Add(FString::Printf(TEXT("%s.%s"), *Node->GetName(), *Pin->PinName.ToString()));
				}
			}
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);

	const TSharedRef<FJsonObject> Report = MakeCompileResultsJson(Blueprint, Results);
	Report->SetArrayField(TEXT("replaced_pins"), ToJsonArray(ReplacedPins));
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::ReplaceVariable(UBlueprint* Blueprint, const FString& OldName, const FString& NewName)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr || OldName.IsEmpty() || NewName.IsEmpty())
	{
		return MakeErrorJson(TEXT("Blueprint is null or a name is empty."));
	}

	const FName OldVarName(*OldName);
	const FName NewVarName(*NewName);

	Blueprint->Modify();

	TArray<UBlueprint*> Dependents;
	FBlueprintEditorUtils::FindDependentBlueprints(Blueprint, Dependents);

	FBlueprintEditorUtils::ReplaceVariableReferences(Blueprint, OldVarName, NewVarName);

	const bool bRemoved = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OldVarName) != INDEX_NONE;
	if (bRemoved)
	{
		FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, OldVarName);
	}

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);

	TArray<FString> DependentNames;
	for (const UBlueprint* Dependent : Dependents)
	{
		DependentNames.Add(Dependent->GetPathName());
	}

	const TSharedRef<FJsonObject> Report = MakeCompileResultsJson(Blueprint, Results);
	Report->SetBoolField(TEXT("removed_old_variable"), bRemoved);
	Report->SetArrayField(TEXT("dependents"), ToJsonArray(DependentNames));
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::FixOrphanedPins(UBlueprint* Blueprint)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	Blueprint->Modify();

	// Rebuilding a node leaves orphans of its own, so the passes repeat until one finds nothing left to do.
	constexpr int32 MaxPasses = 8;
	FPinFixReport Report;
	for (int32 Pass = 0; Pass < MaxPasses; ++Pass)
	{
		const int32 NumChangesBefore = Report.NumChanges();
		FixOrphanedPinsOnce(Blueprint, Report);
		if (Report.NumChanges() == NumChangesBefore)
		{
			break;
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);

	const TSharedRef<FJsonObject> ReportJson = MakeCompileResultsJson(Blueprint, Results);
	ReportJson->SetArrayField(TEXT("moved_links"), ToJsonArray(Report.MovedLinks));
	ReportJson->SetArrayField(TEXT("refused_links"), ToJsonArray(Report.RefusedLinks));
	ReportJson->SetArrayField(TEXT("unmatched_pins"), ToJsonArray(Report.UnmatchedPins));
	ReportJson->SetNumberField(TEXT("removed_pins"), Report.NumRemovedPins);
	return ToJsonString(ReportJson);
}

FString UAssassinsMigrationLibrary::ConvertCustomEventsToOverrides(UBlueprint* Blueprint)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	Blueprint->Modify();

	TArray<FString> ConvertedEvents;
	TArray<FString> Problems;
	AssassinsMigration::ConvertCustomEventsToOverrides(Blueprint, ConvertedEvents, Problems);

	if (ConvertedEvents.Num() > 0)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}

	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Report->SetArrayField(TEXT("converted_events"), ToJsonArray(ConvertedEvents));
	Report->SetArrayField(TEXT("problems"), ToJsonArray(Problems));
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::StripBlueprint(UBlueprint* Blueprint)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	Blueprint->Modify();

	TArray<FString> RemovedInterfaces;
	TArray<FTopLevelAssetPath> InterfacePaths;
	for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
	{
		if (Interface.Interface)
		{
			InterfacePaths.Add(Interface.Interface->GetClassPathName());
			RemovedInterfaces.Add(Interface.Interface->GetPathName());
		}
	}
	for (const FTopLevelAssetPath& InterfacePath : InterfacePaths)
	{
		FBlueprintEditorUtils::RemoveInterface(Blueprint, InterfacePath, /*bPreserveFunctions*/ false);
	}

	int32 RemovedEventGraphNodes = 0;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph == nullptr)
		{
			continue;
		}

		const TArray<UEdGraphNode*> Nodes = Graph->Nodes;
		for (UEdGraphNode* Node : Nodes)
		{
			if (Node)
			{
				FBlueprintEditorUtils::RemoveNode(Blueprint, Node, /*bDontRecompile*/ true);
				++RemovedEventGraphNodes;
			}
		}
	}

	// Keep the first event graph page, empty.
	TArray<UEdGraph*> GraphsToRemove;
	for (int32 Index = 1; Index < Blueprint->UbergraphPages.Num(); ++Index)
	{
		GraphsToRemove.Add(Blueprint->UbergraphPages[Index]);
	}

	TArray<FString> RemovedFunctions;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		// Actor blueprints always have a construction script: keep it, only with its entry.
		if (Graph && (Graph->GetFName() == UEdGraphSchema_K2::FN_UserConstructionScript))
		{
			const TArray<UEdGraphNode*> Nodes = Graph->Nodes;
			for (UEdGraphNode* Node : Nodes)
			{
				if (Node && (Node->GetClass()->GetFName() != NAME_K2NodeFunctionEntry))
				{
					FBlueprintEditorUtils::RemoveNode(Blueprint, Node, /*bDontRecompile*/ true);
				}
			}
			continue;
		}

		GraphsToRemove.Add(Graph);
		RemovedFunctions.Add(GetNameSafe(Graph));
	}

	TArray<FString> RemovedMacros;
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		GraphsToRemove.Add(Graph);
		RemovedMacros.Add(GetNameSafe(Graph));
	}

	GraphsToRemove.Remove(nullptr);
	if (GraphsToRemove.Num() > 0)
	{
		FBlueprintEditorUtils::RemoveGraphs(Blueprint, GraphsToRemove);
	}

	// Event dispatchers are variables too.
	TArray<FName> VariableNames;
	TArray<FString> RemovedVariables;
	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		VariableNames.Add(Variable.VarName);
		RemovedVariables.Add(Variable.VarName.ToString());
	}
	if (VariableNames.Num() > 0)
	{
		FBlueprintEditorUtils::BulkRemoveMemberVariables(Blueprint, VariableNames);
	}

	TArray<UEdGraph*> DelegateGraphs;
	for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs)
	{
		if (Graph)
		{
			DelegateGraphs.Add(Graph);
		}
	}
	if (DelegateGraphs.Num() > 0)
	{
		FBlueprintEditorUtils::RemoveGraphs(Blueprint, DelegateGraphs);
	}

	Blueprint->LastEditedDocuments.Reset();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Root->SetArrayField(TEXT("removed_interfaces"), ToJsonArray(RemovedInterfaces));
	Root->SetNumberField(TEXT("removed_event_graph_nodes"), RemovedEventGraphNodes);
	Root->SetArrayField(TEXT("removed_functions"), ToJsonArray(RemovedFunctions));
	Root->SetArrayField(TEXT("removed_macros"), ToJsonArray(RemovedMacros));
	Root->SetArrayField(TEXT("removed_variables"), ToJsonArray(RemovedVariables));
	Root->SetBoolField(TEXT("is_data_only"), FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint));
	return ToJsonString(Root);
}

FString UAssassinsMigrationLibrary::ReparentBlueprint(UBlueprint* Blueprint, const FString& NewParentClassPath)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	UClass* NewParentClass = LoadClass<UObject>(nullptr, *NewParentClassPath);
	if (NewParentClass == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("Failed to load class '%s'."), *NewParentClassPath));
	}

	if (NewParentClass == Blueprint->ParentClass.Get())
	{
		return CompileBlueprint(Blueprint);
	}

	// Mirrors UBlueprintEditorLibrary::ReparentBlueprint, only to get the compiler results back.
	Blueprint->Modify();
	Blueprint->ParentClass = NewParentClass;

	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint,
		EBlueprintCompileOptions::SkipSave | EBlueprintCompileOptions::UseDeltaSerializationDuringReinstancing | EBlueprintCompileOptions::SkipNewVariableDefaultsDetection,
		&Results);

	return ToJsonString(MakeCompileResultsJson(Blueprint, Results));
}

FString UAssassinsMigrationLibrary::ApplyClassDefaults(UBlueprint* Blueprint, const FString& DefaultsJson)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr || Blueprint->GeneratedClass == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint or its generated class is null."));
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DefaultsJson);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return MakeErrorJson(TEXT("Failed to parse the defaults JSON."));
	}

	// Accepts the output of DumpClassDefaults as well as a flat { "Name": "Value" } object.
	TSharedPtr<FJsonObject> Properties = Root;
	const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
	if (Root->TryGetObjectField(TEXT("properties"), PropertiesObject))
	{
		Properties = *PropertiesObject;
	}

	UClass* Class = Blueprint->GeneratedClass;
	UObject* CDO = Class->GetDefaultObject();
	CDO->Modify();

	TArray<FString> Applied;
	TArray<FString> Missing;
	TArray<FString> Failed;
	TArray<FString> Mismatched;
	TArray<FString> SkippedEditConst;
	TArray<FString> SkippedInstanced;
	int32 NumUnchanged = 0;

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Properties->Values)
	{
		const FString& Key = Pair.Key;

		FString Value;
		if (Pair.Value->Type == EJson::Object)
		{
			Value = Pair.Value->AsObject()->GetStringField(TEXT("value"));
		}
		else if (!Pair.Value->TryGetString(Value))
		{
			Failed.Add(Key + TEXT(": value is not a string"));
			continue;
		}

		FString PropertyName = Key;
		int32 Index = 0;
		int32 BracketIndex = INDEX_NONE;
		if (Key.FindChar(TEXT('['), BracketIndex) && Key.EndsWith(TEXT("]")))
		{
			PropertyName = Key.Left(BracketIndex);
			Index = FCString::Atoi(*Key.Mid(BracketIndex + 1, Key.Len() - BracketIndex - 2));
		}

		const FProperty* Property = Class->FindPropertyByName(FName(*PropertyName));
		if (Property == nullptr || Index < 0 || Index >= Property->ArrayDim)
		{
			Missing.Add(Key);
			continue;
		}

		const FString Current = ExportPropertyValue(Property, CDO, Index);
		if (Current == Value)
		{
			++NumUnchanged;
			continue;
		}

		// A blueprint can't edit these, so a different value means the native class decided otherwise.
		if (Property->HasAnyPropertyFlags(CPF_EditConst))
		{
			SkippedEditConst.Add(FString::Printf(TEXT("%s: baseline %s, current %s"), *Key, *Value, *Current));
			continue;
		}

		if (Property->HasAnyPropertyFlags(CPF_InstancedReference | CPF_ContainsInstancedReference))
		{
			SkippedInstanced.Add(Key);
			continue;
		}

		FStringOutputDevice Errors;
		const TCHAR* Result = Property->ImportText_Direct(*Value, Property->ContainerPtrToValuePtr<void>(CDO, Index), CDO, PPF_None, &Errors);
		if (Result == nullptr)
		{
			Failed.Add(FString::Printf(TEXT("%s: %s"), *Key, *Errors));
			continue;
		}

		const FString Imported = ExportPropertyValue(Property, CDO, Index);
		if (Imported != Value)
		{
			Mismatched.Add(FString::Printf(TEXT("%s: wanted %s, got %s"), *Key, *Value, *Imported));
			continue;
		}

		Applied.Add(Key);
	}

	Blueprint->MarkPackageDirty();

	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Report->SetNumberField(TEXT("num_unchanged"), NumUnchanged);
	Report->SetArrayField(TEXT("applied"), ToJsonArray(Applied));
	Report->SetArrayField(TEXT("missing"), ToJsonArray(Missing));
	Report->SetArrayField(TEXT("failed"), ToJsonArray(Failed));
	Report->SetArrayField(TEXT("mismatched"), ToJsonArray(Mismatched));
	Report->SetArrayField(TEXT("skipped_edit_const"), ToJsonArray(SkippedEditConst));
	Report->SetArrayField(TEXT("skipped_instanced"), ToJsonArray(SkippedInstanced));
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::CompileBlueprint(UBlueprint* Blueprint)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);

	return ToJsonString(MakeCompileResultsJson(Blueprint, Results));
}

FString UAssassinsMigrationLibrary::RemoveNodes(UBlueprint* Blueprint, const TArray<FString>& QualifiedNodeNames)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	Blueprint->Modify();

	// Node names are unique within a graph only: the same name shows up in several graphs.
	TArray<FString> Removed;
	TArray<FString> Unmatched = QualifiedNodeNames;
	for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
	{
		// A copy: removing a node changes the list.
		const TArray<TObjectPtr<UEdGraphNode>> Nodes = Entry.Graph->Nodes;
		for (UEdGraphNode* Node : Nodes)
		{
			if (Node == nullptr)
			{
				continue;
			}

			const FString QualifiedName = FString::Printf(TEXT("%s.%s"), *Entry.Graph->GetName(), *Node->GetName());
			if (!QualifiedNodeNames.Contains(QualifiedName))
			{
				continue;
			}

			FBlueprintEditorUtils::RemoveNode(Blueprint, Node, /*bDontRecompile*/ true);
			Removed.Add(QualifiedName);
			Unmatched.Remove(QualifiedName);
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);

	const TSharedRef<FJsonObject> Report = MakeCompileResultsJson(Blueprint, Results);
	Report->SetArrayField(TEXT("removed_nodes"), ToJsonArray(Removed));
	Report->SetArrayField(TEXT("unmatched_names"), ToJsonArray(Unmatched));
	return ToJsonString(Report);
}

namespace AssassinsMigration
{
	// "<Graph>.<Node>", the way RemoveNodes and the text export name nodes.
	static UEdGraphNode* FindQualifiedNode(const UBlueprint* Blueprint, const FString& QualifiedNodeName)
	{
		for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
		{
			for (UEdGraphNode* Node : Entry.Graph->Nodes)
			{
				if (Node && (FString::Printf(TEXT("%s.%s"), *Entry.Graph->GetName(), *Node->GetName()) == QualifiedNodeName))
				{
					return Node;
				}
			}
		}

		return nullptr;
	}
};

FString UAssassinsMigrationLibrary::AddFunctionCallNode(UBlueprint* Blueprint, const FString& GraphName, const FString& FunctionOwnerClassPath, const FString& FunctionName, int32 PosX, int32 PosY)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	UEdGraph* Graph = nullptr;
	for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
	{
		if (Entry.Graph->GetName() == GraphName)
		{
			Graph = Entry.Graph;
			break;
		}
	}

	if (Graph == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("No graph named %s."), *GraphName));
	}

	const UClass* OwnerClass = LoadObject<UClass>(nullptr, *FunctionOwnerClassPath);
	const UFunction* Function = OwnerClass ? OwnerClass->FindFunctionByName(FName(*FunctionName)) : nullptr;
	if (Function == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("No function %s in %s."), *FunctionName, *FunctionOwnerClassPath));
	}

	Blueprint->Modify();
	Graph->Modify();

	FGraphNodeCreator<UK2Node_CallFunction> NodeCreator(*Graph);
	UK2Node_CallFunction* CallNode = NodeCreator.CreateNode();
	CallNode->SetFromFunction(Function);
	CallNode->NodePosX = PosX;
	CallNode->NodePosY = PosY;
	NodeCreator.Finalize();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	TArray<FString> PinNames;
	for (const UEdGraphPin* Pin : CallNode->Pins)
	{
		PinNames.Add(Pin->PinName.ToString());
	}

	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Report->SetStringField(TEXT("node"), FString::Printf(TEXT("%s.%s"), *Graph->GetName(), *CallNode->GetName()));
	Report->SetArrayField(TEXT("pins"), ToJsonArray(PinNames));
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::LinkPins(UBlueprint* Blueprint, const FString& FromNode, const FString& FromPin, const FString& ToNode, const FString& ToPin)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	UEdGraphNode* SourceNode = FindQualifiedNode(Blueprint, FromNode);
	UEdGraphNode* TargetNode = FindQualifiedNode(Blueprint, ToNode);
	UEdGraphPin* SourcePin = SourceNode ? SourceNode->FindPin(FName(*FromPin)) : nullptr;
	UEdGraphPin* TargetPin = TargetNode ? TargetNode->FindPin(FName(*ToPin)) : nullptr;
	if ((SourcePin == nullptr) || (TargetPin == nullptr))
	{
		return MakeErrorJson(FString::Printf(TEXT("Pin not found: %s %s.%s, %s %s.%s"),
			SourcePin ? TEXT("found") : TEXT("missing"), *FromNode, *FromPin, TargetPin ? TEXT("found") : TEXT("missing"), *ToNode, *ToPin));
	}

	Blueprint->Modify();
	SourceNode->Modify();
	TargetNode->Modify();

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	const bool bLinked = Schema->TryCreateConnection(SourcePin, TargetPin);

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Report->SetBoolField(TEXT("linked"), bLinked);
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::ConvertToValidatedGet(UBlueprint* Blueprint, const FString& QualifiedNodeName)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	UK2Node_VariableGet* GetNode = Cast<UK2Node_VariableGet>(FindQualifiedNode(Blueprint, QualifiedNodeName));
	if (GetNode == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("No variable get node %s."), *QualifiedNodeName));
	}

	// The engine's SetPurity isn't exported and the variation is private: set it as the property it is, then rebuild the
	// pins the way the editor's toggle does.
	const FEnumProperty* VariationProperty = FindFProperty<FEnumProperty>(UK2Node_VariableGet::StaticClass(), TEXT("CurrentVariation"));
	if (VariationProperty == nullptr)
	{
		return MakeErrorJson(TEXT("UK2Node_VariableGet has no CurrentVariation property."));
	}

	Blueprint->Modify();
	GetNode->Modify();
	VariationProperty->GetUnderlyingProperty()->SetIntPropertyValue(VariationProperty->ContainerPtrToValuePtr<void>(GetNode), static_cast<int64>(EGetNodeVariation::ValidatedObject));
	GetNode->ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	TArray<FString> PinNames;
	for (const UEdGraphPin* Pin : GetNode->Pins)
	{
		PinNames.Add(Pin->PinName.ToString());
	}

	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Report->SetStringField(TEXT("node"), QualifiedNodeName);
	Report->SetBoolField(TEXT("validated"), !GetNode->IsNodePure());
	Report->SetArrayField(TEXT("pins"), ToJsonArray(PinNames));
	return ToJsonString(Report);
}

FString UAssassinsMigrationLibrary::AddMacroNode(UBlueprint* Blueprint, const FString& GraphName, const FString& MacroLibraryPath, const FString& MacroName, int32 PosX, int32 PosY)
{
	using namespace AssassinsMigration;

	if (Blueprint == nullptr)
	{
		return MakeErrorJson(TEXT("Blueprint is null."));
	}

	UEdGraph* Graph = nullptr;
	for (const FGraphEntry& Entry : GatherGraphs(Blueprint))
	{
		if (Entry.Graph->GetName() == GraphName)
		{
			Graph = Entry.Graph;
			break;
		}
	}

	if (Graph == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("No graph named %s."), *GraphName));
	}

	const UBlueprint* MacroLibrary = LoadObject<UBlueprint>(nullptr, *MacroLibraryPath);
	UEdGraph* MacroGraph = nullptr;
	if (MacroLibrary)
	{
		for (UEdGraph* Candidate : MacroLibrary->MacroGraphs)
		{
			if (Candidate && (Candidate->GetName() == MacroName))
			{
				MacroGraph = Candidate;
				break;
			}
		}
	}

	if (MacroGraph == nullptr)
	{
		return MakeErrorJson(FString::Printf(TEXT("No macro %s in %s."), *MacroName, *MacroLibraryPath));
	}

	Blueprint->Modify();
	Graph->Modify();

	FGraphNodeCreator<UK2Node_MacroInstance> NodeCreator(*Graph);
	UK2Node_MacroInstance* MacroNode = NodeCreator.CreateNode();
	MacroNode->SetMacroGraph(MacroGraph);
	MacroNode->NodePosX = PosX;
	MacroNode->NodePosY = PosY;
	NodeCreator.Finalize();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	TArray<FString> PinNames;
	for (const UEdGraphPin* Pin : MacroNode->Pins)
	{
		PinNames.Add(Pin->PinName.ToString());
	}

	const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("blueprint"), Blueprint->GetPathName());
	Report->SetStringField(TEXT("node"), FString::Printf(TEXT("%s.%s"), *Graph->GetName(), *MacroNode->GetName()));
	Report->SetArrayField(TEXT("pins"), ToJsonArray(PinNames));
	return ToJsonString(Report);
}
