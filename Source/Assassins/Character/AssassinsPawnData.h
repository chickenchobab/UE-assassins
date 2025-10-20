// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AssassinsPawnData.generated.h"

class UAssassinsAbilitySet;

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Abilities")
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Abilities")
	TArray<TObjectPtr<UAssassinsAbilitySet>> AbilitySets;
};
