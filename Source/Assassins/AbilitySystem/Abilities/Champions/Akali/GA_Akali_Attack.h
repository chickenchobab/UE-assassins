#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Melee.h"
#include "GA_Akali_Attack.generated.h"

class UAssassinsMontageWithTiming;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Akali_Attack : public UGA_Attack_Melee
{
	GENERATED_BODY()

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void OnAttackHit() override;
	virtual void SetMontageToPlay() override;

	void CheckAttackRange();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> PassiveDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsMontageWithTiming> AttackAfterKickMontageSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsMontageWithTiming> PassiveAttackMontageSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double CachedAbilityRange = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool ShouldPlayAttackAfterKick = false;
};
