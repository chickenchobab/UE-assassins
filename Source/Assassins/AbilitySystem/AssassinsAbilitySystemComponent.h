// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

#include "AssassinsAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBeginStatusDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndStatusDelegate, bool, bResumePaused);

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:

	UAssassinsAbilitySystemComponent(const FObjectInitializer& ObjectInitializer);

	//~UAbilitySystemComponent interface
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags) override;
	//~End of UAbilitySystemComponent interface

	void AbilityInputTagPressed(FGameplayTag& InputTag);
	void AbilityInputTagReleased(FGameplayTag& InputTag);

	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	void ClearAbilityInput();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability", DisplayName = "CancelAbilities", meta = (ScriptName = "CancelAbilities"))
	void K2_CancelAbilities(FGameplayTag WithTag, FGameplayTag WithoutTag);

	void CancelAbilitiesWithCancelledByTag(const FGameplayTagContainer* WithTags, UGameplayAbility* Ignore);

	// Uses a gameplay effect to add the specified dynamic granted tag.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	FActiveGameplayEffectHandle AddDynamicTagGameplayEffect(FGameplayTag Tag);
	// Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void RemoveDynamicTagGameplayEffect(FGameplayTag Tag);

public:

    // Me: Delegates for status changes, which are called in anim notifies

    UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Assassins|Status")
    FOnBeginStatusDelegate OnBeginChanneling;
    UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Assassins|Status")
    FOnEndStatusDelegate OnEndChanneling;

protected:

	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

protected:

	// Handles to abilities that had their input pressed this frame.
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// Handles to abilities that had their input released this frame.
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	// Handles to abilities that have their input held.
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
