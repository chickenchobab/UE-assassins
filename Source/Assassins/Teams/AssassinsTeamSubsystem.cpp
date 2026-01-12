// Fill out your copyright notice in the Description page of Project Settings.


#include "Teams/AssassinsTeamSubsystem.h"
#include "Teams/AssassinsTeamAgentInterface.h"
#include "Teams/AssassinsTeamInfo.h"
#include "Teams/AssassinsTeamAsset.h"
#include "Player/AssassinsPlayerState.h"
#include "Character/AssassinsCharacterWithAbilities.h"

UAssassinsTeamSubsystem::UAssassinsTeamSubsystem()
{
}

const AAssassinsTeamInfo* UAssassinsTeamSubsystem::GetTeamInfo(int32 TeamId) const
{
	for (const AAssassinsTeamInfo* TeamInfo : Teams)
	{
		if (TeamInfo)
		{
			if (TeamId == TeamInfo->GetTeamId())
			{
				return TeamInfo;
			}
		}
	}

	return nullptr;
}

int32 UAssassinsTeamSubsystem::GetTeamOfIndex(int32 TeamIndex) const
{
	if (TeamIndex < Teams.Num())
	{
		return Teams[TeamIndex]->GetTeamId();
	}

	return INDEX_NONE;
}

int32 UAssassinsTeamSubsystem::GetNumTeams() const
{
	return Teams.Num();
}

bool UAssassinsTeamSubsystem::RegisterTeamInfo(AAssassinsTeamInfo* TeamInfo)
{
	if (!ensure(TeamInfo))
	{
		return false;
	}

	const int32 TeamId = TeamInfo->GetTeamId();
	if (ensure(TeamId != INDEX_NONE))
	{
		Teams.AddUnique(TeamInfo);
		return true;
	}

	return false;
}

bool UAssassinsTeamSubsystem::UnregisterTeamInfo(AAssassinsTeamInfo* TeamInfo)
{
	if (!ensure(TeamInfo))
	{
		return false;
	}

	const int32 TeamId = TeamInfo->GetTeamId();
	if (ensure(TeamId != INDEX_NONE))
	{
		Teams.Remove(TeamInfo);
	}

	return false;
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

void UAssassinsTeamSubsystem::GetMinionInfo(TSubclassOf<AAssassinsCharacterWithAbilities>& OutMinionClass, FTransform& OutMinionSpawnTransform, int32& TeamIndex, int32& MinionTypeIndex, bool bAdvanceTeamIndex, bool bAdvanceMinionTypeIndex) const
{
	if (Teams.IsEmpty())
	{
		return;
	}

	if (bAdvanceTeamIndex)
	{
		TeamIndex = TeamIndex + 1;
	}
	TeamIndex %= Teams.Num();

	if (const AAssassinsTeamInfo* TeamInfo = Teams[TeamIndex])
	{
		if (const UAssassinsTeamAsset* TeamAsset = TeamInfo->GetTeamAsset())
		{
			if (TeamAsset->MinionClasses.IsEmpty())
			{
				return;
			}

			if (bAdvanceMinionTypeIndex)
			{
				MinionTypeIndex = MinionTypeIndex + 1;
			}
			MinionTypeIndex %= TeamAsset->MinionClasses.Num();

			OutMinionClass = TeamAsset->MinionClasses[MinionTypeIndex];
		}
		OutMinionSpawnTransform = TeamInfo->GetTeamBaseTransform();
	}
}
