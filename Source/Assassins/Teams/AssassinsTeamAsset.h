// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "AssassinsTeamAsset.generated.h"

class AAssassinsCharacterWithAbilities;

// Represents the display information for team definitions
UCLASS(BlueprintType)
class UAssassinsTeamAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, float> ScalarParameters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, FLinearColor> ColorParameters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<AAssassinsCharacterWithAbilities>> MinionClasses;
};
