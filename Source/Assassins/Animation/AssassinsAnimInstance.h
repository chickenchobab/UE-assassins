// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"

#include "AssassinsAnimInstance.generated.h"

class UAbilitySystemComponent;

/**
 * The base game animation instance class used by this project
 */
UCLASS()
class UAssassinsAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:

	UAssassinsAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);

	//~UAnimInstance interface
	virtual void NativeInitializeAnimation() override;
	//~End of UAnimInstance interface

protected:

	// Gameplay tags that can be mapped to blueprint variables. The variables will automatically update as the tags are added or removed.
	// These should be used instead of manually querying for the gameplay tags.
	UPROPERTY(EditDefaultsOnly, Category = "GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;
};
