// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "Teams/AssassinsTeamAgentInterface.h"

#include "AssassinsPlayerState.generated.h"

class UAssassinsPawnData;
class UAssassinsAbilitySystemComponent;
class AAssassinsPlayerController;
class UAssassinsExperienceDefinition;
class UAssassinsExperienceEntry;
class UAssassinsHealthSet;
class UAssassinsCombatSet;
class AAssassinsCharacter;

/**
 * 
 */
UCLASS()
class ASSASSINS_API AAssassinsPlayerState : public AModularPlayerState, public IAbilitySystemInterface, public IAssassinsTeamAgentInterface
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

	UFUNCTION(BlueprintCallable, Category = "Assassins|PlayerState")
	void OnChampionSelected(const UAssassinsPawnData* InPawnData);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_OnChampionSelected(const UAssassinsPawnData* InPawnData);

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	void SetPawnData(const UAssassinsPawnData* InPawnData, bool bShouldApplyAbilitySets = true);
	void ApplyAbilitySets();

	//~AActor interface
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	//~End of APlayerState interface

	//~IAssassinsTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~End of IAssassinsTeamAgentInterface interface

private:
	void OnExperienceLoaded(const UAssassinsExperienceDefinition* CurrentExperience);

protected:
	UPROPERTY(VisibleAnywhere, Replicated)
	TObjectPtr<const UAssassinsPawnData> PawnData;

private:
	UPROPERTY(VisibleAnywhere, Category = "Assassins|PlayerState")
	TObjectPtr<UAssassinsAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UAssassinsHealthSet> HealthSet;

	UPROPERTY()
	TObjectPtr<const UAssassinsCombatSet> CombatSet;

	UPROPERTY(ReplicatedUsing=OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	bool bChampionSelected;

private:

	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);
};
