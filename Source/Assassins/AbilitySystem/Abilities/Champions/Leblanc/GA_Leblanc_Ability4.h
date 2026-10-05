// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "GA_Leblanc_Ability4.generated.h"

/**
 * UGA_Leblanc_Ability4
 *
 * Leblanc R, Mimic. Casts again the last of her Ability1 to Ability3, through the event its mimic answers(Leblanc's
 * skill state keeps which one), with the event the ultimate came with. It lasts as long as the mimic does. Pressing the
 * ability again sends the recast to the mimic, on the owning client only: the mimic of Ability2 returns on it, once it
 * landed. Every press is sent, as one made during the dash of the mimic is lost. Was the GA_Leblanc_Ability4 blueprint.
 */
UCLASS(Abstract)
class UGA_Leblanc_Ability4 : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Ability4(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~End of UGameplayAbility interface

private:

	// Owning client: waits for the input to be let go, then pressed again.
	void WaitForRecast();

	UFUNCTION()
	void OnMimicStarted();

	UFUNCTION()
	void OnMimicEnded();

	UFUNCTION()
	void OnRecastReleased(float TimeHeld);

	UFUNCTION()
	void OnRecastPressed(float TimeWaited);
};
