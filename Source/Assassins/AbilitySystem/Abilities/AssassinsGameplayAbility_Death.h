// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AssassinsGameplayAbility_Death.generated.h"

/**
 * Gameplay ability used for handling death.
 * Ability is activated automatically via the "Event.Death" ability trigger tag
 */
UCLASS(Abstract)
class UAssassinsGameplayAbility_Death : public UAssassinsGameplayAbility
{
	GENERATED_BODY()
	
public:

	UAssassinsGameplayAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void StartDeath();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void FinishDeath();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Assassins|Ability")
	bool bCancelAllAbilities;
};
