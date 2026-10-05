#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "GA_ActivateAttack.generated.h"

UCLASS(Abstract)
class UGA_ActivateAttack : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_ActivateAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:

	UFUNCTION()
	void OnAttackingEnded();

	UFUNCTION()
	void OnAttackReady();

	UPROPERTY()
	FGameplayEventData AttackEventData;
};
