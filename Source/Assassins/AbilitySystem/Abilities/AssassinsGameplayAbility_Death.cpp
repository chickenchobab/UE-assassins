// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/AssassinsGameplayAbility_Death.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsHealthComponent.h"

UAssassinsGameplayAbility_Death::UAssassinsGameplayAbility_Death(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	bCancelAllAbilities = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// Add the ability trigger tag as default to the CDO.
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = AssassinsGameplayTags::Event_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UAssassinsGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	if (bCancelAllAbilities)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->CancelAbilities(nullptr, nullptr, this);
		}
	}

	SetCanBeCanceled(false);

	StartDeath();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAssassinsGameplayAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	check(ActorInfo);

	// Always try to finish the death when the ability ends in case the ability doesn't.
	// This won't do anything if the death hasn't been started.
	FinishDeath();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAssassinsGameplayAbility_Death::StartDeath()
{
	if (UAssassinsHealthComponent* HealthComponent = UAssassinsHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == EAssassinsDeathState::NotDead)
		{
			HealthComponent->StartDeath();
		}
	}
}

void UAssassinsGameplayAbility_Death::FinishDeath()
{
	if (UAssassinsHealthComponent* HealthComponent = UAssassinsHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == EAssassinsDeathState::DeathStarted)
		{
			HealthComponent->FinishDeath();
		}
	}
}