// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AssassinsGameplayAbility_UnitTargeted.generated.h"

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsGameplayAbility_UnitTargeted : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~End of UGameplayAbility interface

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Ability")
	float AbilityRange;
};
