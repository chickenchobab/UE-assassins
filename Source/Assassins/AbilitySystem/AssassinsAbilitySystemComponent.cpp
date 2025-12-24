// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AssassinsLogCategories.h"
#include "System/AssassinsAssetManager.h"
#include "System/AssassinsGameData.h"
#include "AssassinsGameplayTags.h"

UAssassinsAbilitySystemComponent::UAssassinsAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

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
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
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

int32 UAssassinsAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (!HasMatchingGameplayTag(AssassinsGameplayTags::Status_Channeling))
	{
		return Super::HandleGameplayEvent(EventTag, Payload);
	}

	return 0;
}
void UAssassinsAbilitySystemComponent::AddDynamicGameplayEffect(FGameplayTag Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UAssassinsAssetManager::GetSubclass(UAssassinsGameData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogAssassinsAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to find DynamicTagGameplayEffect [%s]."), *UAssassinsGameData::Get().DynamicTagGameplayEffect.GetAssetName());
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DynamicTagGE, 1.0f, MakeEffectContext());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	if (!Spec)
	{
		UE_LOG(LogAssassinsAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to make outgoing spec for [%s]."), *GetNameSafe(DynamicTagGE));
		return;
	}

	Spec->DynamicGrantedTags.AddTag(Tag);

	ApplyGameplayEffectSpecToSelf(*Spec);
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
