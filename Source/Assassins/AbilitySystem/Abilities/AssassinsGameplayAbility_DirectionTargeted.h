// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AssassinsGameplayAbility_DirectionTargeted.generated.h"

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsGameplayAbility_DirectionTargeted : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

    UAssassinsGameplayAbility_DirectionTargeted(const FObjectInitializer& ObjectInitializer);
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Ability")
	float AbilityRange;
};
