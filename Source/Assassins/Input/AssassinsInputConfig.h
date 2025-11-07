// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "AssassinsInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;


USTRUCT()
struct FInputMappingContextAndPriority
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	// Higher priority input mappings will be prioritized over mappings with a lower priority.
	UPROPERTY(EditAnywhere, Category = "Input")
	int32 Priority = 0;
};


USTRUCT(BlueprintType)
struct FAssassinsInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Assassins|Pawn")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Pawn")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag);
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FAssassinsInputAction> NativeInputActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FAssassinsInputAction> AbilityInputActions;
};
