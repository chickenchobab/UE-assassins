// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Melee.h"
#include "GA_Akali_Attack.generated.h"

class UAssassinsMontageWithTiming;
class UGameplayEffect;

/**
 * UGA_Akali_Attack
 *
 * Akali basic attack. Her passive turns the next attack into an empowered one: it reaches twice as far, plays its
 * own montage and carries extra damage. The attack that follows her kick(Ability3) has its own montage as well.
 * Was the GA_Akali_Attack blueprint.
 */
UCLASS(Abstract)
class UGA_Akali_Attack : public UGA_Attack_Melee
{
	GENERATED_BODY()

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_Attack interface
	virtual void OnAttackHit() override;
	virtual void SetMontageToPlay() override;
	//~End of UGA_Attack interface

	// The empowered attack reaches further, so the range is doubled before the avatar starts walking.
	void CheckAttackRange();

protected:

	// What the empowered attack adds on top of the usual damage.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> PassiveDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsMontageWithTiming> AttackAfterKickMontageSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsMontageWithTiming> PassiveAttackMontageSet;

	// The range the attack goes back to once it is done, since the empowered attack changes it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double CachedAbilityRange = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool ShouldPlayAttackAfterKick = false;
};
