// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModularGameState.h"
#include "AssassinsGameState.generated.h"

class UAssassinsExperienceStateComponent;
class UAssassinsCameraMode;

/**
 * 
 */
UCLASS()
class ASSASSINS_API AAssassinsGameState : public AModularGameStateBase
{
	GENERATED_BODY()

public:
	AAssassinsGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	TSubclassOf<UAssassinsCameraMode> GetExperienceCameraMode() const;


private:
	UPROPERTY(VisibleAnywhere, Category = "Assassins|GameState")
	TObjectPtr<UAssassinsExperienceStateComponent> ExperienceComponent;
	
};
