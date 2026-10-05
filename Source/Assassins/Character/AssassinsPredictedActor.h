// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "AssassinsPredictedActor.generated.h"

/**
 * AAssassinsPredictedActor
 *
 * An actor an ability spawns on the owning client and on the server alike, owned by the avatar: the owning client shows
 * its own at once, and the copy of the server's that replicates to it stays out of the way there. Akali's shroud and
 * Zed's shadows.
 */
UCLASS(Abstract)
class ASSASSINS_API AAssassinsPredictedActor : public AActor
{
	GENERATED_BODY()

public:

	// Whether this is the server's copy on the owning client, which has a copy of its own: it shows nothing and does
	// nothing there. Was the ShouldSkipComsmetic function of B_Shroud and B_ZedShadow.
	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	bool ShouldSkipCosmetic() const;
};
