#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Melee.h"
#include "GA_Zed_Attack.generated.h"

class UAssassinsMontageWithTiming;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Zed_Attack : public UGA_Attack_Melee
{
	GENERATED_BODY()

protected:

	virtual void PrePlayMontage() override;
	virtual void OnAttackHit() override;
	virtual void SetMontageToPlay() override;

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	bool CanApplyPassive(const AActor* TargetActor) const;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> PassiveDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsMontageWithTiming> PassiveAttackMontageData;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool IsPassiveApplied = false;
};
