// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "AssassinsPlayerState.generated.h"

class UAssassinsPawnData;
class UAssassinsAbilitySystemComponent;
class AAssassinsPlayerController;
class UAssassinsExperienceDefinition;
class UAssassinsHealthSet;

/**
 * 
 */
UCLASS()
class ASSASSINS_API AAssassinsPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AAssassinsPlayerState(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintCallable, Category = "Assassins|PlayerState")
	AAssassinsPlayerController* GetAssassinsPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|PlayerState")
	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponent() const { return AbilitySystemComponent; }
	//~Begin IAbilitySystemInterface interface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End IAbilitySystemInterface interface

	static const FName NAME_AssassinsAbilityReady;

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	void SetPawnData(const UAssassinsPawnData* InPawnData);

	//~AActor interface
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

private:
	void OnExperienceLoaded(const UAssassinsExperienceDefinition* CurrentExperience);

protected:
	UPROPERTY()
	TObjectPtr<const UAssassinsPawnData> PawnData;

private:
	UPROPERTY(VisibleAnywhere, Category = "Assassins|PlayerState")
	TObjectPtr<UAssassinsAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UAssassinsHealthSet> HealthSet;
};
