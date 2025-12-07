// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AssassinsGameplayAbility_UnitTargeted.h"
#include "Character/AssassinsCharacter.h"
#include "Player/AssassinsPlayerController.h"

UAssassinsGameplayAbility_UnitTargeted::UAssassinsGameplayAbility_UnitTargeted(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    AbilityRange = 0.0f;
}

void UAssassinsGameplayAbility_UnitTargeted::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
