// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "AssassinsTeamSubsystem.generated.h"

class AAssassinsPlayerState;

UENUM(BlueprintType)
enum class EAssassinsTeamComparison : uint8
{
	// Both actors are members of the same team
	OnSameTeam,

	// The actors are members of opposing teams
	DifferentTeams,

	// One (or both) of the actors was invalid or not part of any team
	InvalidArgument
};

/**
 * A subsystem for easy access to team information for team-based actors
 */
UCLASS()
class ASSASSINS_API UAssassinsTeamSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UAssassinsTeamSubsystem();

	// Returns the team this object belongs to, or INDEX_NONE if it is not part of a team
	int32 FindTeamFromObject(const UObject* TestObject) const;

	// Returns the associated player state for this actor, or INDEX_NONE if it is not associated with a player
	const AAssassinsPlayerState* FindPlayerStateFromActor(const AActor* PossibleTeamActor) const;

	// Compare the teams of two actors and returns a value indicating if they are on same teams, different teams, or one/both are invalid
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = Teams, meta = (ExpandEnumAsExecs = ReturnValue))
	EAssassinsTeamComparison CompareTeams(const UObject* A, const UObject* B, int32& OutTeamIdA, int32& OutTeamIdB) const;
};
