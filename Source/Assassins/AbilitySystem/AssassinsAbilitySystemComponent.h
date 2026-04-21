// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

#include "AssassinsAbilitySystemComponent.generated.h"

class UAssassinsHeroComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetChasingCompleted);

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:

	UAssassinsAbilitySystemComponent(const FObjectInitializer& ObjectInitializer);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//~UAbilitySystemComponent interface
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags) override;
	virtual int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) override;
	//~End of UAbilitySystemComponent interface

	void AbilityInputTagPressed(FGameplayTag& InputTag);
	void AbilityInputTagReleased(FGameplayTag& InputTag);

	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	void ClearAbilityInput();

	bool IsCurrentEventAbilityInput() const;

	AActor* GetCursorTargetFromHeroComponent() const;

	void TryActivateAbilitiesOnSpawn();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability", DisplayName = "CancelAbilities", meta = (ScriptName = "CancelAbilities"))
	void K2_CancelAbilities(FGameplayTag WithTag, FGameplayTag WithoutTag);

	void CancelAbilitiesWithCancelledByTag(const FGameplayTagContainer* WithTags, UGameplayAbility* Ignore);

	// Uses a gameplay effect to add the specified dynamic granted tag.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	FActiveGameplayEffectHandle AddDynamicTagGameplayEffect(FGameplayTag Tag);
	// Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void RemoveDynamicTagGameplayEffect(FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	FGameplayEffectContextHandle GetContextFromGameplayEffectSpec(const FGameplayEffectSpec& Spec) const;

public:
	
	UPROPERTY(BlueprintAssignable, Category = "Assassins|Ability")
	FOnTargetChasingCompleted OnTargetChasingCompleted;
		 
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

private:

	UPROPERTY(Transient)
	FGameplayTag CurrentEventTag;
};
