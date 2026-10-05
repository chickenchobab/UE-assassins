// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AITypes.h"
#include "GameplayTagContainer.h"
#include "Navigation/PathFollowingComponent.h"
#include "GA_UnitTargeted.generated.h"

class UAnimMontage;
class UAssassinsMontageWithTiming;
class UAssassinsTargetChasingComponent;

/**
 * UGA_UnitTargeted
 *
 * Ability aimed at a unit. The avatar chases the target until it is within AbilityRange, both sides meet at a
 * network sync point, and then the montage plays. Was the GA_UnitTargeted blueprint.
 */
UCLASS(Abstract)
class UGA_UnitTargeted : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_UnitTargeted(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	// Sends the avatar to the target. The server waits for the arrival, the client waits for the server to tell it.
	virtual void MoveToTarget();

	// Steps of the montage, so that the abilities deriving from this one can take part.
	virtual void PlayMontage();
	virtual void PrePlayMontage();
	virtual void PostPlayMontage() {}
	virtual void OnMontageComplete();
	virtual void OnMontageCanceled();

	// Picks the montage to play and its hit timing out of MontageDataArray.
	virtual void SetMontageToPlay();

	// Whether the avatar keeps chasing the target once the ability is done. Only a child says otherwise.
	virtual bool ShouldKeepFollowingTarget() const;

	// What the ability holds while it waits for the other side before its cast: from the moment it is cast at a target in
	// range(or from its arrival at the target) until its montage starts, where it puts on what it holds while it casts.
	// That wait is a round trip: without these, an ability pressed meanwhile starts and cuts the cast short. None by
	// default, as for the attack.
	virtual FGameplayTagContainer GetPendingCastTags() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void StopControllerMove();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void ResetTargetState();

	// Rotation from the avatar to the target, on the XY plane.
	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	FRotator GetTargetLookAtRotation(const AActor* TargetActor) const;

	void WaitForNetSync();

	// Where the avatar arrives at the target. A child takes these over to do something else on the way to the montage.
	UFUNCTION()
	virtual void OnMoveComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UFUNCTION()
	virtual void OnServerReachedToTarget();

	UFUNCTION()
	virtual void OnNetSync();

	// Bound to the montage task, so that a child playing its own montage still lands in OnMontageComplete/OnMontageCanceled.
	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCanceled();

	// How close the avatar has to be to the target before the ability plays.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double AbilityRange = 0.0;

	// Champions are only answered as a target when this is set. The other units always are.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	bool ShouldTargetChampion = false;

	// While the avatar has any of these it keeps following the target. Read by the attack, through ShouldKeepFollowingTarget.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<FGameplayTag> ActionBlockingTags;

	// One of these is picked every time the ability plays.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<TObjectPtr<UAssassinsMontageWithTiming>> MontageDataArray;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double MontageRate = 1.0;

	// Set by SetMontageToPlay out of MontageDataArray. An ability that always plays the same montage sets it here instead.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> MontageToPlay;

	// When the montage hits, from the montage data of the montage being played.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	double HitEventTime = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AActor> AbilityTargetActor;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsTargetChasingComponent> CachedTargetChasingComponent;

private:

	// Within AbilityRange of the target, as the chase is over at once.
	bool IsTargetInRange() const;

	void AddPendingCastTags();
	void RemovePendingCastTags();

	// The pending cast tags are on: they come off once.
	bool bPendingCastTagsAdded = false;
};
