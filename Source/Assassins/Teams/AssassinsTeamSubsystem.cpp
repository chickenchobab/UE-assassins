// Fill out your copyright notice in the Description page of Project Settings.


#include "Teams/AssassinsTeamSubsystem.h"
#include "Teams/AssassinsTeamAgentInterface.h"
#include "Player/AssassinsPlayerState.h"

UAssassinsTeamSubsystem::UAssassinsTeamSubsystem()
{
}

int32 UAssassinsTeamSubsystem::FindTeamFromObject(const UObject* TestObject) const
{
	// See if it's directly a team agent
	if (const IAssassinsTeamAgentInterface* ObjectWithTeamInterface = Cast<IAssassinsTeamAgentInterface>(TestObject))
	{
		return GenericTeamIdToInteger(ObjectWithTeamInterface->GetGenericTeamId());
	}

	if (const AActor* TestActor = Cast<const AActor>(TestObject))
	{
		// See if the instigator is a team actor
		if (const IAssassinsTeamAgentInterface* InstigatorWithTeamInterface = Cast<IAssassinsTeamAgentInterface>(TestActor->GetInstigator()))
		{
			return GenericTeamIdToInteger(InstigatorWithTeamInterface->GetGenericTeamId());
		}

		// Fall back to finding the associated player state
		if (const AAssassinsPlayerState* AssassinsPS = FindPlayerStateFromActor(TestActor))
		{
			return GenericTeamIdToInteger(AssassinsPS->GetGenericTeamId());
		}
	}

	return INDEX_NONE;
}

const AAssassinsPlayerState* UAssassinsTeamSubsystem::FindPlayerStateFromActor(const AActor* PossibleTeamActor) const
{
	if (PossibleTeamActor != nullptr)
	{
		if (const APawn* Pawn = Cast<const APawn>(PossibleTeamActor))
		{
			if (AAssassinsPlayerState* AssassinsPS = Pawn->GetPlayerState<AAssassinsPlayerState>())
			{
				return AssassinsPS;
			}
		}
		else if (const AController* PC = Cast<const AController>(PossibleTeamActor))
		{
			if (AAssassinsPlayerState* AssassinsPS = Cast<AAssassinsPlayerState>(PC->PlayerState))
			{
				return AssassinsPS;
			}
		}
		else if (const AAssassinsPlayerState* AssassinsPS = Cast<const AAssassinsPlayerState>(PossibleTeamActor))
		{
			return AssassinsPS;
		}
	}

	return nullptr;
}

EAssassinsTeamComparison UAssassinsTeamSubsystem::CompareTeams(const UObject* A, const UObject* B, int32& TeamIdA, int32& TeamIdB) const
{
	TeamIdA = FindTeamFromObject(Cast<const AActor>(A));
	TeamIdB = FindTeamFromObject(Cast<const AActor>(B));

	if ((TeamIdA == INDEX_NONE) || (TeamIdB == INDEX_NONE))
	{
		return EAssassinsTeamComparison::InvalidArgument;
	}
	else
	{
		return (TeamIdA == TeamIdB) ? EAssassinsTeamComparison::OnSameTeam : EAssassinsTeamComparison::DifferentTeams;
	}
}
