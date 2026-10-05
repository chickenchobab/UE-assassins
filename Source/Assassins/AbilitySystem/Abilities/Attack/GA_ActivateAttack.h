// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "GA_ActivateAttack.generated.h"

/**
 * UGA_ActivateAttack
 *
 * Asks for the next attack on the target the event names, a live character or a team base: once the attack going on
 * is over, it sends the event the attack answers(Event.Attack) with the same payload. Runs on the owning client only,
 * the attack is what talks to the server. Was the GA_ActivateAttack blueprint.
 */
UCLASS(Abstract)
class UGA_ActivateAttack : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_ActivateAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

private:

	UFUNCTION()
	void OnAttackingEnded();

	UFUNCTION()
	void OnAttackReady();

	// The event the ability came with, passed on to the attack.
	UPROPERTY()
	FGameplayEventData AttackEventData;
};
