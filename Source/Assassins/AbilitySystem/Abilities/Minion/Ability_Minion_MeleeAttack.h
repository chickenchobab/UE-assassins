#pragma once

#include "AbilitySystem/Abilities/Attack/Ability_MeleeAttack.h"
#include "Ability_Minion_MeleeAttack.generated.h"

UCLASS(Abstract)
class UAbility_Minion_MeleeAttack : public UAbility_MeleeAttack
{
	GENERATED_BODY()

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void SetMontageToPlay() override;
};
