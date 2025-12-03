// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AssassinsGameplayAbility_UnitTargeted.h"
#include "Character/AssassinsCharacter.h"
#include "Player/AssassinsPlayerController.h"

void UAssassinsGameplayAbility_UnitTargeted::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}
