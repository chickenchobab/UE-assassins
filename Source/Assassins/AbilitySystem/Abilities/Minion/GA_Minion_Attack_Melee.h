#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Melee.h"
#include "GA_Minion_Attack_Melee.generated.h"

UCLASS(Abstract)
class UGA_Minion_Attack_Melee : public UGA_Attack_Melee
{
	GENERATED_BODY()

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void SetMontageToPlay() override;
};
