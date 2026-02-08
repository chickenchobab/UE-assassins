// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "GameplayEffectExecutionCalculation_BlueprintBase.generated.h"

/**
 * 
 */
UCLASS()
class ASSASSINS_API UGameplayEffectExecutionCalculation_BlueprintBase : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
protected:

	UFUNCTION(BlueprintPure, Category = "Calculation")
	UAbilitySystemComponent* GetTargetAbilitySystemComponentFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const;

	UFUNCTION(BlueprintPure, Category = "Calculation")
	UAbilitySystemComponent* GetSourceAbilitySystemComponentFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const;

	UFUNCTION(BlueprintPure, Category = "Calculation")
	AActor* GetTargetAvatarActorFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const;

	UFUNCTION(BlueprintPure, Category = "Calculation")
	AActor* GetSourceAvatarActorFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const;

	UFUNCTION(BlueprintPure, Category = "Calculation")
	FGameplayEffectContextHandle GetEffectContextFromParameters(const FGameplayEffectCustomExecutionParameters& Parameters) const;
};
