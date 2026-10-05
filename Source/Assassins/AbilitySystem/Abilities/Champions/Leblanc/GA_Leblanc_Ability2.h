// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GA_Leblanc_Ability2.generated.h"

class UAbilityTask_DashTo;
class UAbilityTask_WaitDelay;
class UGameplayEffect;

/**
 * UGA_Leblanc_Ability2
 *
 * Leblanc W, Distortion. Leblanc dashes to the cursor and damages the enemies where she lands. For a while she may
 * recast to return where she started. The mimic of her ultimate(Ability.Ability4) returns on the recast of the
 * ultimate instead of its own input.
 *
 * The hit of the dash is predicted by the owning client, with the only sync point of the activation. The return goes
 * over its own replicated event: the input events of the engine for the ability itself, and a custom event for the
 * mimic, since the ultimate tells only the owning client. The server listens from the start, so it may hear the
 * return while its own dash is still going(see DoReturn). The owning client returns once it landed, on a press made
 * since the press that cast the ability was let go(also one still down as it lands), but not during the cast of
 * another ability. Was the GA_Leblanc_Ability2 blueprint, whose return synced a second time and took the signal of the
 * first one, which left the server stuck.
 */
UCLASS(Abstract)
class UGA_Leblanc_Ability2 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Ability2(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_LocationTargeted_Immediate interface
	virtual void PrePlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}
	//~End of UGA_LocationTargeted_Immediate interface

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float DashSpeed = 0.0f;

	// How long Leblanc may return after the dash.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float ReturnDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Marks where Leblanc returns to, until she does or the time is up. The mimic has a cue of its own.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayTag ReturnLocationCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> ReturnMontage;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle DamageSpecHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector ReturnLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FRotator ReturnRotation = FRotator::ZeroRotator;

	// Every enemy is hit once.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TSet<TObjectPtr<AActor>> ActorsToIgnore;

private:

	bool IsMimic() const;

	void HandleDashComplete(const FVector& EndLocation);
	void ApplyDashDamage();

	// The owning client from the end of the dash, the server from the start.
	void WaitForRecast();

	// The return by the input of the ability. Its release is watched for from the start on both sides; the press, by the
	// owning client once it landed and by the server once the release came.
	void WaitForRecastInputRelease();
	void WaitForReturnPress(bool bTestAlreadyPressed);

	// Runs on both sides, in the prediction key of the recast.
	void DoReturn();
	void FinishDashEarly();
	void StartReturn();

	UFUNCTION()
	void OnDashFinished();

	UFUNCTION()
	void OnDashCancelled();

	UFUNCTION()
	void OnDashSynced();

	UFUNCTION()
	void OnRecastInputReleased(float TimeHeld);

	UFUNCTION()
	void OnRecastInputPressed();

	UFUNCTION()
	void OnUltimateRecast(FGameplayEventData Payload);

	UFUNCTION()
	void OnReturnRequested();

	UFUNCTION()
	void OnReturnDurationFinished();

	UFUNCTION()
	void OnReturnMontageEnded();

	UPROPERTY()
	TObjectPtr<UAbilityTask_DashTo> DashTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> ReturnDurationTask;

	// Where the dash ends, taken for the hit on both sides.
	FVector DashEndLocation = FVector::ZeroVector;

	bool bDashFinished = false;
	bool bDashSynced = false;

	// The press that cast the ability was let go: the input down after that is a press for the return.
	bool bRecastInputReleased = false;

	bool bReturnRequested = false;
	bool bReturnStarted = false;
};
