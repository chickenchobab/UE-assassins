// Fill out your copyright notice in the Description page of Project Settings.


#include "Teams/AssassinsTeamCreationComponent.h"
#include "Teams/AssassinsTeamBaseActor.h"
#include "Teams/AssassinsTeamInfo.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AssassinsLogCategories.h"

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

void UAssassinsTeamCreationComponent::ServerCreateTeam(int32 TeamId, UAssassinsTeamAsset* TeamAsset, const FTransform& TeamBaseTransform)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAssassinsTeamInfo* Team = GetWorld()->SpawnActor<AAssassinsTeamInfo>(Params);
	Team->SetTeamId(TeamId);
	Team->SetTeamAsset(TeamAsset);
	Team->SetTeamBaseTransform(TeamBaseTransform);
}

#endif