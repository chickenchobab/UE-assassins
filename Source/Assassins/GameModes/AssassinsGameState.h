// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModularGameState.h"
#include "AssassinsGameState.generated.h"

class UAssassinsExperienceStateComponent;
class UAssassinsCameraMode;
class UAssassinsPawnData;

/**
 * 
 */
UCLASS()
class ASSASSINS_API AAssassinsGameState : public AModularGameStateBase
{
	GENERATED_BODY()

public:
	AAssassinsGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	TSubclassOf<UAssassinsCameraMode> GetExperienceCameraMode() const;

	void SetInLobby(bool bLobby) { bInLobby = bLobby; }
	bool IsGameInLobby() const { return bInLobby; }

private:

	UFUNCTION()
	void OnRep_bInLobby();

private:
	UPROPERTY(VisibleAnywhere, Category = "Assassins|GameState")
	TObjectPtr<UAssassinsExperienceStateComponent> ExperienceComponent;

	UPROPERTY(ReplicatedUsing=OnRep_bInLobby)
	bool bInLobby;
};
