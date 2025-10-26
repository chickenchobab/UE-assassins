// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AssassinsAbilitySet.generated.h"

class UAssassinsGameplayAbility;
class UGameplayEffect;
class UAttributeSet;

/**
 * 
 */
USTRUCT(BlueprintType)
struct FAssassinsAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAssassinsGameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};


USTRUCT(BlueprintType)
struct FAssassinsAbilitySet_GameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.f;
};


USTRUCT(BlueprintType)
struct FAssassinsAbilitySet_AttributeSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeSet> AttributeSet = nullptr;
};


UCLASS()
class ASSASSINS_API UAssassinsAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	TArray<FAssassinsAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	UPROPERTY(EditDefaultsOnly)
	TArray<FAssassinsAbilitySet_GameplayEffect> GrantedGameplayEffects;

	UPROPERTY(EditDefaultsOnly)
	TArray<FAssassinsAbilitySet_AttributeSet> GrantedAttributeSets;
};
