// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AssassinsAbilitySet.generated.h"

class UAssassinsGameplayAbility;
class UGameplayEffect;
class UAttributeSet;
class UAssassinsAbilitySystemComponent;
struct FGameplayAbilitySpecHandle;
struct FActiveGameplayEffectHandle;


/**
 * FAssassinsAbilitySet_GameplayAbility
 * 
 *  Data used by the ability set to grant gameplay abilities.
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


/**
* FAssassinsAbilitySet_GameplayEffect
* 
*  Data used by the ability set to grant gameplay effects.
*/
USTRUCT(BlueprintType)
struct FAssassinsAbilitySet_GameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.f;
};


/**
* FAssassinsAbilitySet_AttributeSet
*
*  Data used by the ability set to grant attribute sets.
*/
USTRUCT(BlueprintType)
struct FAssassinsAbilitySet_AttributeSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeSet> AttributeSet = nullptr;
};


/**
* FAssassinsAbilitySet_GrantedHandles
* 
*  Data used to store handles to what has been granted by the ability set.
*/
USTRUCT(BlueprintType)
struct FAssassinsAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);
	// Me: What is it for?
	void AddAttributeSet(UAttributeSet* Set);

protected:

	UPROPERTY()
	TArray< FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY()
	TArray< FActiveGameplayEffectHandle> GameplayEffectHandles;

	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
};

/**
* UAssassinsAbilitySet
* 
* Non-mutable data asset used to grant gameplay abilities and gameplay effects.
*/
UCLASS()
class UAssassinsAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Grants the ability set to the specified ability system component.
	// The returned handles can be used later to take away anything that was granted.
	void GiveToAbilitySystem(UAssassinsAbilitySystemComponent* InASC, FAssassinsAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

protected:
	
	UPROPERTY(EditDefaultsOnly)
	TArray<FAssassinsAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	UPROPERTY(EditDefaultsOnly)
	TArray<FAssassinsAbilitySet_GameplayEffect> GrantedGameplayEffects;

	UPROPERTY(EditDefaultsOnly)
	TArray<FAssassinsAbilitySet_AttributeSet> GrantedAttributeSets;
};
