// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AssassinsLogCategories.h"
#include "System/AssassinsAssetManager.h"
#include "System/AssassinsGameData.h"
#include "AssassinsGameplayTags.h"
#include "Animation/AssassinsAnimInstance.h"
#include "Character/AssassinsHeroComponent.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ABILITYINPUT, "Event.AbilityInput");

UAssassinsAbilitySystemComponent::UAssassinsAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentEventTag = TAG_EVENT_ABILITYINPUT;
}

void UAssassinsAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UAssassinsAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (UAssassinsAnimInstance* AssassinsAnimInst = Cast<UAssassinsAnimInstance>(ActorInfo->GetAnimInstance()))
	{
		AssassinsAnimInst->InitializeWithAbilitySystem(this);
	}
}

void UAssassinsAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, BlockTags, bExecuteCancelTags, CancelTags);

	if (bExecuteCancelTags)
	{
		CancelAbilitiesWithCancelledByTag(&AbilityTags, RequestingAbility);
	}
}

int32 UAssassinsAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	const FGameplayTag Tag = CurrentEventTag;
	CurrentEventTag = EventTag;

	// CurrentEventTag is used by the activated ability to know whether
	// it is triggered by input or not
	int32 ReturnValue = Super::HandleGameplayEvent(EventTag, Payload);

	CurrentEventTag = Tag;

	return ReturnValue;
}

void UAssassinsAbilitySystemComponent::AbilityInputTagPressed(FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void UAssassinsAbilitySystemComponent::AbilityInputTagReleased(FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void UAssassinsAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	// Me: TODO ASC has ability blocking tag

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	// 
	// Process all abilities that activate when the input is held.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const UAssassinsGameplayAbility* AssassinsAbilityCDO = Cast<UAssassinsGameplayAbility>(AbilitySpec->Ability);
				if (AssassinsAbilityCDO && AssassinsAbilityCDO->GetActivationPolicy() == EAssassinsAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	//
	// Process all abilities that had their input pressed this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
					const UAssassinsGameplayAbility* AssassinsAbilityCDO = Cast<UAssassinsGameplayAbility>(AbilitySpec->Ability);
					if (AssassinsAbilityCDO && AssassinsAbilityCDO->GetActivationPolicy() == EAssassinsAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	//
	// Try to activate all the abilities that from presses and holds.
	// We do it all at once so that held inputs don't activate the ability
	// and then also send a input event to the ability because of the press.
	// Set cursor info to the hit results of event data for
	// the replicated abilities in the server to have the same cursor info.

	FGameplayEventData EventData;
	EventData.ContextHandle = MakeEffectContext();
	FHitResult GroundHit;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, GroundHit))
		{
			EventData.ContextHandle.AddHitResult(GroundHit);
		}
	}
	EventData.Target = GetCursorTargetFromHeroComponent();

	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TriggerAbilityFromGameplayEvent(
			AbilitySpecHandle,
			AbilityActorInfo.Get(),
			TAG_EVENT_ABILITYINPUT,
			&EventData,
			*this
		);
	}

	//
	// Process all abilities that had their input released this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	//
	// Clear the cached ability handles.
	//
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UAssassinsAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

bool UAssassinsAbilitySystemComponent::IsCurrentEventAbilityInput() const
{
	return CurrentEventTag == TAG_EVENT_ABILITYINPUT;
}

AActor* UAssassinsAbilitySystemComponent::GetCursorTargetFromHeroComponent() const
{
	// This function is not for an ability activation time(the initial cursor position is handled to the ability spec), 
	// but for dynamic cursor logic in runtime, like triggering another ability that requires new cursor position or interacting with some objects. 

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (!PC->IsLocalController())
		{
			return nullptr;
		}
	}

	if (AActor* MyAvatarActor = GetAvatarActor())
	{
		if (UAssassinsHeroComponent* HeroComponent = MyAvatarActor->FindComponentByClass<UAssassinsHeroComponent>())
		{
			return HeroComponent->GetCursorTarget();
		}
	}

	return nullptr;
}

void UAssassinsAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (const UAssassinsGameplayAbility* AssassinsAbilityCDO = Cast<UAssassinsGameplayAbility>(AbilitySpec.Ability))
		{
			AssassinsAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
		}
	}
}

void UAssassinsAbilitySystemComponent::K2_CancelAbilities(FGameplayTag WithTag, FGameplayTag WithoutTag)
{
	FGameplayTagContainer WithTags(WithTag);
	FGameplayTagContainer WithoutTags(WithoutTag);

	CancelAbilities(&WithTags, &WithoutTags);
}

void UAssassinsAbilitySystemComponent::CancelAbilitiesWithCancelledByTag(const FGameplayTagContainer* WithTags, UGameplayAbility* Ignore)
{
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.IsActive() || Spec.Ability == nullptr)
		{
			continue;
		}

		if (UAssassinsGameplayAbility* AssassinsAbility = Cast<UAssassinsGameplayAbility>(Spec.Ability))
		{
			const FGameplayTagContainer& CancelledByTags = AssassinsAbility->GetCancelledByTags();
			if (CancelledByTags.HasAny(*WithTags))
			{
				CancelAbilitySpec(Spec, Ignore);
			}
		}
	}
}

FActiveGameplayEffectHandle UAssassinsAbilitySystemComponent::AddDynamicTagGameplayEffect(FGameplayTag Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UAssassinsAssetManager::GetSubclass(UAssassinsGameData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogAssassinsAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to find DynamicTagGameplayEffect [%s]."), *UAssassinsGameData::Get().DynamicTagGameplayEffect.GetAssetName());
		return FActiveGameplayEffectHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DynamicTagGE, 1.0f, MakeEffectContext());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	if (!Spec)
	{
		UE_LOG(LogAssassinsAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to make outgoing spec for [%s]."), *GetNameSafe(DynamicTagGE));
		return FActiveGameplayEffectHandle();
	}

	Spec->DynamicGrantedTags.AddTag(Tag);

	return ApplyGameplayEffectSpecToSelf(*Spec);
}

void UAssassinsAbilitySystemComponent::RemoveDynamicTagGameplayEffect(FGameplayTag Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UAssassinsAssetManager::GetSubclass(UAssassinsGameData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogAssassinsAbilitySystem, Warning, TEXT("RemoveDynamicTagGameplayEffect: Unable to find gameplay effect [%s]."), *UAssassinsGameData::Get().DynamicTagGameplayEffect.GetAssetName());
		return;
	}

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
	Query.EffectDefinition = DynamicTagGE;

	RemoveActiveEffects(Query);
}

FGameplayEffectContextHandle UAssassinsAbilitySystemComponent::GetContextFromGameplayEffectSpec(const FGameplayEffectSpec& Spec) const
{
	return Spec.GetContext();
}

void UAssassinsAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

    // We don't support UGameplayAbility::bReplicateInputDirectly.
    // Use replicated events instead so that the WaitInputPress ability task works.
    if (Spec.IsActive())
    {
PRAGMA_DISABLE_DEPRECATION_WARNINGS
        const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
        FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

        // Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
        InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
    }
}

void UAssassinsAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

    // We don't support UGameplayAbility::bReplicateInputDirectly.
    // Use replicated events instead so that the WaitInputRelease ability task works.
    if (Spec.IsActive())
    {
PRAGMA_DISABLE_DEPRECATION_WARNINGS
        const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
        FPredictionKey OriginalPredictionKey = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
PRAGMA_ENABLE_DEPRECATION_WARNINGS

        // Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputReleased event to the server.
        InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
    }
}