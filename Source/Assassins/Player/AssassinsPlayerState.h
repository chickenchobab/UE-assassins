// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "AssassinsPlayerState.generated.h"

class UAssassinsPawnData;
class UAssassinsAbilitySystemComponent;
class AAssassinsPlayerController;

/**
 * 
 */
UCLASS()
class ASSASSINS_API AAssassinsPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TObjectPtr<const UAssassinsPawnData> PawnData;
	
	UFUNCTION(BlueprintCallable, Category = "Assassins|PlayerState")
	AAssassinsPlayerController* GetAssassinsPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|PlayerState")
	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponent() const { return AbilitySystemComponent; }
	//~Begin IAbilitySystemInterface interface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End IAbilitySystemInterface interface

private:
	UPROPERTY(VisibleAnywhere, Category = "Assassins|PlayerState")
	TObjectPtr<UAssassinsAbilitySystemComponent> AbilitySystemComponent;

	//TODO : HealthSet and CombatSet
};
