// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/AssassinsCharacter.h"
#include "AssassinsCharacterWithAbilities.generated.h"

class UAssassinsAbilitySet;

/**
 * AssassinsCharacter typically gets the ability system component from the possessing player state.
 * This represents a character with a self-contained ability system component.
 * Me: This is for a bot or testable character.
 */
UCLASS(Blueprintable)
class ASSASSINS_API AAssassinsCharacterWithAbilities : public AAssassinsCharacter
{
	GENERATED_BODY()
	
public:
    AAssassinsCharacterWithAbilities();

    //~AActor interface
    virtual void PostInitializeComponents() override;
    //~End of AActor interface

    //~IAbilitySystemInterface interface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    //~End of IAbilitySystemInterface interface

    //~IAssassinsTeamAgentInterface interface
    virtual void SetGenericTeamId(const FGenericTeamId& NewID) override;
    //~End of IAssassinsTeamAgentInterface interface


public:

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Ability")
    TObjectPtr<UAssassinsAbilitySet> AbilitySet;

private:

    UPROPERTY(VisibleAnywhere, Category = "Assassins|PlayerState")
    TObjectPtr<UAssassinsAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY()
    TObjectPtr<const class UAssassinsHealthSet> HealthSet;
    UPROPERTY()
    TObjectPtr<const class UAssassinsCombatSet> CombatSet;
};
