// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Melee.h"
#include "GA_Zed_Attack.generated.h"

class UAssassinsMontageWithTiming;
class UGameplayEffect;

/**
 * UGA_Zed_Attack
 *
 * Zed basic attack. Against a target below half health his passive lands with the attack: it deals extra damage
 * and plays its own montage. Was the GA_Zed_Attack blueprint.
 */
UCLASS(Abstract)
class UGA_Zed_Attack : public UGA_Attack_Melee
{
	GENERATED_BODY()

protected:

	//~UGA_Attack interface
	virtual void PrePlayMontage() override;
	virtual void OnAttackHit() override;
	virtual void SetMontageToPlay() override;
	//~End of UGA_Attack interface

	// Whether the target is worth the passive: hurt enough, and not marked by it already.
	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	bool CanApplyPassive(const AActor* TargetActor) const;

protected:

	// The extra damage the passive deals.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> PassiveDamageEffectClass;

	// The montage the attack plays when the passive lands with it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsMontageWithTiming> PassiveAttackMontageData;

	// Decided before the montage starts, so that the montage and the hit agree on it.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool IsPassiveApplied = false;
};
