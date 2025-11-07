// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "AssassinsPawnData.generated.h"

class UAssassinsAbilitySet;
class UAssassinsInputConfig;

/**
 * 
 */
UCLASS(MinimalAPI, BlueprintType, Meta = (DisplayName = "Assassins Pawn Data", ShortTooltip = "Data asset used to define a pawn."))
class UAssassinsPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	// Class to instantiate for this pawn (should usually derive from AAssassinsPawn or AAssassinsCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Abilities")
	TSubclassOf<APawn> PawnClass;

	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Abilities")
	TArray<TObjectPtr<UAssassinsAbilitySet>> AbilitySets;

	// Input configuration used by player controlled pawns to create input mappings and bind input actions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Abilities")
	TObjectPtr<UAssassinsInputConfig> InputConfig;
};
