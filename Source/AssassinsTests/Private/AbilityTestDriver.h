// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class AActor;
class FAbilityTestSteps;
struct FAbilityTestPlayer;

/** Where a player's ability inputs go: the location they hit, and the actor under the cursor. */
struct FAbilityTestAim
{
	FVector Location = FVector::ZeroVector;

	// The copy in the aiming player's client world.
	TWeakObjectPtr<AActor> Target;
};

/**
 * FAbilityTestDriver
 *
 * Plays a player's part on its client: aims in place of the mouse(AAssassinsPlayerController::SetAimOverride) and
 * presses the ability inputs the way the input component does. The inputs then go through the player controller's
 * input processing and the ability system as they would in a game: activation event, prediction, input replication.
 */
struct FAbilityTestDriver
{
	// The ground under the actor, and the actor.
	static FAbilityTestAim AimAt(AActor* ClientTarget);

	static void SetAim(const FAbilityTestPlayer& Player, const FAbilityTestAim& Aim);
	static void ClearAim(const FAbilityTestPlayer& Player);

	static void PressInput(const FAbilityTestPlayer& Player, const FGameplayTag& InputTag);
	static void ReleaseInput(const FAbilityTestPlayer& Player, const FGameplayTag& InputTag);

	// Adds the steps that press the input of the player(by its order in the session config) and release it on the next
	// frame, as a quick tap would.
	static void AddTapSteps(FAbilityTestSteps& Steps, int32 PlayerIndex, const FGameplayTag& InputTag);

	// Moves the player's champion a frame's worth the way, on its client, as holding the move click does
	// (UAssassinsHeroComponent::OnSetDestinationTriggered). Called every frame for as long as it walks.
	static void AddMoveInput(const FAbilityTestPlayer& Player, const FVector& Direction);
};
