// Fill out your copyright notice in the Description page of Project Settings.


#include "Teams/AssassinsTeamCreationComponent.h"
#include "Teams/AssassinsTeamBaseActor.h"
#include "Teams/AssassinsTeamInfo.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "Player/AssassinsPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "AssassinsLogCategories.h"
#include "GameModes/AssassinsGameMode.h"

UAssassinsTeamCreationComponent::UAssassinsTeamCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAssassinsTeamCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the experience load to complete.
	AGameStateBase* GameState = GetGameState<AGameStateBase>();
	UAssassinsExperienceStateComponent* ExperienceComponent = GameState->FindComponentByClass<UAssassinsExperienceStateComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(FOnAssassinsExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void UAssassinsTeamCreationComponent::OnExperienceLoaded(const UAssassinsExperienceDefinition* Experience)
{
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateTeams();
		ServerAssignPlayersToTeams();
	}
#endif
}

#if WITH_SERVER_CODE

void UAssassinsTeamCreationComponent::GetTeamBaseTransform(FTransform& BaseTransform, int32 TeamId, const TArray<AActor*>& TeamBases, bool& bFoundTeamBase)
{
	for (const AActor* TeamBase : TeamBases)
	{
		if (const AAssassinsTeamBaseActor* TeamBaseActor = Cast<AAssassinsTeamBaseActor>(TeamBase))
		{
			if (TeamBaseActor->TeamId == TeamId)
			{
				BaseTransform = TeamBaseActor->GetActorTransform();
				return;
			}
		}
	}

	bFoundTeamBase = false;
}

int32 UAssassinsTeamCreationComponent::GetLeastPopulatedTeamID() const
{
	const int32 NumTeams = TeamsToCreate.Num();
	if (NumTeams > 0)
	{
		TMap<int32, int32> TeamMemberCounter;
		TeamMemberCounter.Reserve(NumTeams);
		for (const auto& KVP : TeamsToCreate)
		{
			const int32 TeamId = KVP.Key;
			TeamMemberCounter.Add(TeamId, 0);
		}

		AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (AAssassinsPlayerState* AssassinsPS = Cast<AAssassinsPlayerState>(PS))
			{
				const int32 PlayerTeamId = GenericTeamIdToInteger(AssassinsPS->GetGenericTeamId());

				if ((PlayerTeamId != INDEX_NONE) && !AssassinsPS->IsInactive())
				{
					TeamMemberCounter[PlayerTeamId]++;
				}
			}
		}

		int32 BestTeamId = INDEX_NONE;
		uint32 BestPlayerCount = GameState->PlayerArray.Num();
		for (const auto& KVP : TeamMemberCounter)
		{
			const int32 TestTeamId = KVP.Key;
			const uint32 TestTeamPlayerCount = KVP.Value;

			if (TestTeamPlayerCount < BestPlayerCount)
			{
				BestTeamId = KVP.Key;
				BestPlayerCount = KVP.Value;
			}
			else if (TestTeamPlayerCount == BestPlayerCount)
			{
				if ((TestTeamId < BestTeamId) || (BestTeamId == INDEX_NONE))
				{
					BestTeamId = TestTeamId;
					BestPlayerCount = TestTeamPlayerCount;
				}
			}
		}

		return BestTeamId;
	}

	return INDEX_NONE;
}

void UAssassinsTeamCreationComponent::ServerCreateTeams()
{
	// Get minion spawn position from the team bases(nexuses) in the map
	TArray<AActor*> TeamBases;
	UGameplayStatics::GetAllActorsOfClass(this, AAssassinsTeamBaseActor::StaticClass(), TeamBases);

	if (TeamBases.Num() != TeamsToCreate.Num())
	{
		UE_LOG(LogAssassinsTeams, Error, TEXT("The number of teams specified is different from that of team base located on the map [%s]"), *GetNameSafe(GetWorld()->PersistentLevel));
		return;
	}

	for (const auto& KVP : TeamsToCreate)
	{
		const int32 TeamId = KVP.Key;
		
		FTransform TeamBaseTransform;
		bool bFoundBase = true;
		GetTeamBaseTransform(TeamBaseTransform, TeamId, TeamBases, bFoundBase);

		if (!bFoundBase)
		{
			UE_LOG(LogAssassinsTeams, Error, TEXT("There is no valid team base of the team[%d] on the map [%s]"), TeamId, *GetNameSafe(GetWorld()->PersistentLevel));
			return;
		}

		ServerCreateTeam(TeamId, KVP.Value, TeamBaseTransform);
	}
}

void UAssassinsTeamCreationComponent::ServerAssignPlayersToTeams()
{
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AAssassinsPlayerState* AssassinsPS = Cast<AAssassinsPlayerState>(PS))
		{
			ServerChooseTeamForPlayer(AssassinsPS);
		}
	}

	// Can listen for new players logging in(especially bots)
	AAssassinsGameMode* GameMode = GetGameMode<AAssassinsGameMode>();
	check(GameMode);
	GameMode->OnGameModePlayerInitialized.AddUObject(this, &ThisClass::OnPlayerInitialized);
}

void UAssassinsTeamCreationComponent::ServerChooseTeamForPlayer(AAssassinsPlayerState* PS)
{
	const int32 TeamId = GetLeastPopulatedTeamID();
	PS->SetGenericTeamId(IntegerToGenericTeamId(TeamId));
}

void UAssassinsTeamCreationComponent::ServerCreateTeam(int32 TeamId, UAssassinsTeamAsset* TeamAsset, const FTransform& TeamBaseTransform)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAssassinsTeamInfo* Team = GetWorld()->SpawnActor<AAssassinsTeamInfo>(Params);
	Team->SetTeamId(TeamId);
	Team->SetTeamAsset(TeamAsset);
	Team->SetTeamBaseTransform(TeamBaseTransform);
}

void UAssassinsTeamCreationComponent::OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer)
{
	check(NewPlayer);
	check(NewPlayer->PlayerState);
	if (AAssassinsPlayerState* AssassinsPS = Cast<AAssassinsPlayerState>(NewPlayer->PlayerState))
	{
		ServerChooseTeamForPlayer(AssassinsPS);
	}
}

#endif