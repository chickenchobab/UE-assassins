// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/WorldSettings.h"
#include "AssassinsWorldSettings.generated.h"

class UAssassinsExperienceDefinition;

/**
 * The default world settings object, used primarily to set the default gameplay experience to use when playing on this map
 */
UCLASS()
class AAssassinsWorldSettings : public AWorldSettings
{
	GENERATED_BODY()
	
public:
	AAssassinsWorldSettings(const FObjectInitializer& ObjectInitializer);

	FPrimaryAssetId GetDefaultGameplayExperience() const;

protected:
	// The default experience to use when a server opens this map if it is not overridden by the user-facing experience
	UPROPERTY(EditDefaultsOnly, Category = GameMode)
	TSoftClassPtr<UAssassinsExperienceDefinition> DefaultGameplayExperience;
};
