// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Melee.h"
#include "GA_Minion_Attack_Melee.generated.h"

/**
 * UGA_Minion_Attack_Melee
 *
 * Melee minion basic attack. The range and the montages come from the minion, since every kind of minion has
 * its own. Was the GA_Minion_Attack_Melee blueprint.
 */
UCLASS(Abstract)
class UGA_Minion_Attack_Melee : public UGA_Attack_Melee
{
	GENERATED_BODY()

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~End of UGameplayAbility interface

	//~UGA_Attack interface
	virtual void SetMontageToPlay() override;
	//~End of UGA_Attack interface
};
