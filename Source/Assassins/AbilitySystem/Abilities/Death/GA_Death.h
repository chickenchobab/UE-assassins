// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility_Death.h"
#include "GA_Death.generated.h"

class USceneComponent;

/**
 * UGA_Death
 *
 * Death of a unit: hides what floats over it and plays a death montage, one of the character's or the base's.
 * The ability ends with the montage, and ending it finishes the death. Was the GA_Death blueprint.
 */
UCLASS(Abstract)
class UGA_Death : public UAssassinsGameplayAbility_Death
{
	GENERATED_BODY()

public:

	UGA_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~End of UGameplayAbility interface

	// Hides the indicator and plays the death montage.
	virtual void PlayDeath();

	// The death montage is over, however it stopped. Ends the ability.
	virtual void OnMontageEnd();

	// What shows the health and the name over the unit, hidden as the unit dies.
	UPROPERTY(EditDefaultsOnly, Category = "Assassins|Ability")
	TSubclassOf<USceneComponent> IndicatorComponentClass;

private:

	UFUNCTION()
	void HandleMontageEnded();

	// A montage both blends out and completes: the end is handled once.
	bool bMontageEnded = false;
};
