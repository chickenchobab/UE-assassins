// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/WorldSettings.h"
#include "AssassinsWorldSettings.generated.h"

class UAssassinsExperienceDefinition;

/**
 * 
 */
UCLASS()
class ASSASSINS_API AAssassinsWorldSettings : public AWorldSettings
{
	GENERATED_BODY()
	
public:
	FPrimaryAssetId GetDefaultGameplayExperience() const;

protected:
	// The default experience to use when a server opens this map if it is not overridden by the user-facing experience
	UPROPERTY(EditDefaultsOnly, Category = GameMode)
	TSoftClassPtr<UAssassinsExperienceDefinition> DefaultGameplayExperience;
};
