// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AssassinsGameplayAbility_LocationTargeted.generated.h"

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsGameplayAbility_LocationTargeted : public UAssassinsGameplayAbility
{
	GENERATED_BODY()
	
public:

    UAssassinsGameplayAbility_LocationTargeted(const FObjectInitializer& ObjectInitializer);

protected:

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Ability")
    float AbilityRange;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Ability")
    float EffectRadius;
};
