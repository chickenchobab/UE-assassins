// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "GA_LocationTargeted_Immediate.generated.h"

class UAnimMontage;

/**
 * UGA_LocationTargeted_Immediate
 *
 * Ability aimed at a location, cast where the cursor points without walking there first.
 * Was the GA_LocationTargeted_Immediate blueprint.
 */
UCLASS(Abstract)
class UGA_LocationTargeted_Immediate : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_LocationTargeted_Immediate(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~End of UGameplayAbility interface

	// Reads where the cursor pointed out of the event, and works out the rotation and the location the ability may reach.
	virtual void EvaluateCursorTransform(const FGameplayEventData& EventData);

	// Steps of the montage, so that the abilities deriving from this one can take part.
	virtual void PlayMontage();
	virtual void PrePlayMontage();
	virtual void PostPlayMontage() {}
	virtual void OnMontageComplete();
	virtual void OnMontageCancelled();

private:

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCancelled();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> MontageToPlay;

	// How far from the avatar the ability reaches.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double AbilityRange = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double EffectRadius = 0.0;

	// Where the cursor pointed. The Z is kept, it can be on a character.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector CursorLocation = FVector::ZeroVector;

	// The same location, brought within AbilityRange of the avatar.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector CursorLocationClamped = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FRotator CursorLookAtRotation = FRotator::ZeroRotator;
};
