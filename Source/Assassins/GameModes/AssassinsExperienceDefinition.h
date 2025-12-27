// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"

#include "AssassinsExperienceDefinition.generated.h"

class UAssassinsPawnData;
class UGameFeatureAction;
class UAssassinsExperienceActionSet;
class UAssassinsCameraMode;


/**
 * 
 */
UCLASS(BlueprintType, Const)
class ASSASSINS_API UAssassinsExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// List of Game Feature Plugins this experience wants to have active
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<FString> GameFeaturesToEnable;

	/** The default pawn class to spawn for players */
	//@TODO: Make soft?
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TObjectPtr<const UAssassinsPawnData> DefaultPawnData;

	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TSubclassOf<UAssassinsCameraMode> CameraMode;

	// List of actions to perform as this experience is loaded/activated/deactivated/unloaded
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Actions")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	// List of additional action sets to compose into this experience
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<TObjectPtr<UAssassinsExperienceActionSet>> ActionSets;

};
