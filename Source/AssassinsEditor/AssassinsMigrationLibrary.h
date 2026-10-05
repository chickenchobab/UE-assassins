// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssassinsMigrationLibrary.generated.h"

class UBlueprint;

/**
 * UAssassinsMigrationLibrary
 *
 * Editor-only helpers used to migrate blueprint gameplay abilities to native classes.
 * Exposed to Python(unreal.AssassinsMigrationLibrary) so that the ue-editor MCP server(Tools/ue_mcp) can drive them.
 * Anything larger than a list of strings is returned as JSON or plain text.
 */
UCLASS()
class UAssassinsMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// Object paths of every blueprint whose generated class derives from the given class(e.g. "/Script/GameplayAbilities.GameplayAbility").
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static TArray<FString> FindBlueprintsDerivedFrom(const FString& BaseClassPath);

	// Names of the packages referencing the given package.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static TArray<FString> GetReferencers(const FString& PackageName);

	// Class hierarchy, interfaces, entry points and graphs of the blueprint, as JSON.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString DescribeBlueprint(UBlueprint* Blueprint);

	// Member variables(type, flags, replication, metadata and default value), as JSON.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ExportBlueprintVariables(UBlueprint* Blueprint);

	// Every graph as the text the editor puts on the clipboard(T3D). Full fidelity, hard to read.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ExportBlueprintGraphsAsText(UBlueprint* Blueprint);

	// Every graph as a compact listing which follows the execution flow. Meant to be read when writing the native class.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ExportBlueprintGraphsAsListing(UBlueprint* Blueprint);

	// Effective default values of the class, inherited ones included, as JSON.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString DumpClassDefaults(const FString& ClassPath);

	// Property values of an object(e.g. a data asset instance), in the same format as DumpClassDefaults.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString DumpObjectProperties(UObject* Object);

	// Moves blueprint members to a native parent, keeping the blueprint's logic:
	// removes the given graphs, reparents to NewParentClassPath(when not empty), turns the custom events the native parent
	// now declares into overrides, then compiles. The compiler itself drops the variables the native parent declares with
	// the same name and property type, and points their references to the native ones. Returns a JSON report.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString RebaseBlueprint(UBlueprint* Blueprint, const FString& NewParentClassPath, const TArray<FString>& GraphsToRemove);

	// Points the references to OldName, in the blueprint and in the loaded blueprints depending on it, to NewName, then removes
	// the variable OldName. Fixes the "X_0" variables the compiler makes when it can't match a blueprint variable with the native
	// one of the same name(it happens when the parent blueprint fails to compile). Returns the compiler results as JSON.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ReplaceVariable(UBlueprint* Blueprint, const FString& OldName, const FString& NewName);

	// Moves the links of orphaned pins to the live pin of the same name on the same node, when the schema accepts the connection,
	// then removes the orphans left without links. A node reconstruction leaves an orphan when it can't match the old pin,
	// e.g. when a variable's type became one of its parent classes. Compiles and returns a JSON report.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString FixOrphanedPins(UBlueprint* Blueprint);

	// Sets a property of an object from its text form(the format DumpObjectProperties writes), with the editor's change notifications.
	// Reaches properties Python can't, e.g. of struct types not exposed to Blueprint. Returns the value before and after as JSON.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString SetObjectProperty(UObject* Object, const FString& PropertyName, const FString& Value);

	// Points the pin default values(e.g. the class of an IsA node) set to OldObjectPath to NewObjectPath, then compiles.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ReplacePinDefaultObject(UBlueprint* Blueprint, const FString& OldObjectPath, const FString& NewObjectPath);

	// Replaces the custom events whose names a native parent declares as blueprint events with overrides of them, keeping every link.
	// A custom event can't share its name with a native function, so this is needed after moving an event to C++. Returns a JSON report.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ConvertCustomEventsToOverrides(UBlueprint* Blueprint);

	// Removes every graph, variable and interface so that only the class defaults remain. Returns a JSON report.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString StripBlueprint(UBlueprint* Blueprint);

	// Same as UBlueprintEditorLibrary::ReparentBlueprint, but returns the compiler results as JSON.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ReparentBlueprint(UBlueprint* Blueprint, const FString& NewParentClassPath);

	// Restores default values by property name from the output of DumpClassDefaults. Returns a JSON report.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ApplyClassDefaults(UBlueprint* Blueprint, const FString& DefaultsJson);

	// Compiles the blueprint and returns the compiler results as JSON.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString CompileBlueprint(UBlueprint* Blueprint);

	// Removes the given nodes, named "<Graph>.<Node>" as the text export names them(e.g. EventGraph.K2Node_CustomEvent_2),
	// breaking their links, then compiles. For logic that is no longer used where removing whole graphs would take too much.
	// Returns the compiler results as JSON, with the nodes removed and the names that matched nothing.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString RemoveNodes(UBlueprint* Blueprint, const TArray<FString>& QualifiedNodeNames);

	// Adds a call to the function(e.g. a native one the blueprint inherits) to the graph, at the given position. Links
	// nothing, see LinkPins, and doesn't compile. Returns a JSON report with the new node as "<Graph>.<Node>".
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString AddFunctionCallNode(UBlueprint* Blueprint, const FString& GraphName, const FString& FunctionOwnerClassPath, const FString& FunctionName, int32 PosX, int32 PosY);

	// Links two pins, their nodes named "<Graph>.<Node>" as the text export names them, when the schema accepts the
	// connection. Doesn't compile. Returns a JSON report.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString LinkPins(UBlueprint* Blueprint, const FString& FromNode, const FString& FromPin, const FString& ToNode, const FString& ToPin);

	// Turns a pure get of an object variable, named "<Graph>.<Node>" as the text export names it, into a validated get,
	// as the editor's "Convert to Validated Get" does: exec pins "execute" in, "then"(Is Valid) and "else"(Is Not Valid)
	// out. Links nothing, see LinkPins, and doesn't compile. Returns a JSON report with the node's pins.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString ConvertToValidatedGet(UBlueprint* Blueprint, const FString& QualifiedNodeName);

	// Adds an instance of the macro, e.g. IsValid of /Engine/EditorBlueprintResources/StandardMacros.StandardMacros, to the
	// graph at the given position. Links nothing, see LinkPins, and doesn't compile. Returns a JSON report with the new node
	// as "<Graph>.<Node>" and its pins.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Migration")
	static FString AddMacroNode(UBlueprint* Blueprint, const FString& GraphName, const FString& MacroLibraryPath, const FString& MacroName, int32 PosX, int32 PosY);
};
