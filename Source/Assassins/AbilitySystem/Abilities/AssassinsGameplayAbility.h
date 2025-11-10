// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AssassinsGameplayAbility.generated.h"

class UAssassinsAbilitySystemComponent;
class AAssassinsPlayerController;
class AAssassinsCharacter;

/**
 * EAssassinsAbilityActivationPolicy
 * 
 * Defines how an ability is meant to activate.
 */
UENUM(BlueprintType)
enum class EAssassinsAbilityActivationPolicy : uint8
{
	// Try to activate the ability when the input is triggered.
	OnInputTriggered,

	// Continually try to activate the ability while the input is active.
	WhileInputActive,

	// Try to activate the ability when an avatar is assigned.
	OnSpawn
};

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UAssassinsGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Assassins|Ability")
	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	AAssassinsPlayerController* GetAssassinsPlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	AAssassinsCharacter* GetAssassinsCharacterFromActorInfo() const;

	EAssassinsAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	
protected:
	
	// Defines how this ability is meant to activate.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability Activation")
	EAssassinsAbilityActivationPolicy ActivationPolicy;

};
