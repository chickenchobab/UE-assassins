// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"

#include "AssassinsGameData.generated.h"

#define UE_API ASSASSINS_API

class UGameplayEffect;

/**
 * 
 */
UCLASS(MinimalAPI, BlueprintType, Const, Meta = (DisplayName = "Assassins Game Data", ShortTooltip = "Data asset containing global game data."))
class UAssassinsGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UE_API UAssassinsGameData();

	// Returns the loaded game data.
	UE_API static const UAssassinsGameData& Get();

public:

	// Gameplay effect used to add and remove dynamic tags.
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects")
	TSoftClassPtr<UGameplayEffect> DynamicTagGameplayEffect;
};

#undef UE_API
