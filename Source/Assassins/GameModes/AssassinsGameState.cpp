// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/AssassinsGameState.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "GameModes/AssassinsGameMode.h"
#include "UI/Frontend/AssassinsFrontendStateComponent.h"
#include "Player/AssassinsPlayerState.h"
#include "Character/AssassinsPawnData.h"
#include "Net/UnrealNetwork.h"

AAssassinsGameState::AAssassinsGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	ExperienceComponent = CreateDefaultSubobject<UAssassinsExperienceStateComponent>(TEXT("ExperienceComponent"));

	bInLobby = false;
}

void AAssassinsGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAssassinsGameState, bInLobby);
}

TSubclassOf<UAssassinsCameraMode> AAssassinsGameState::GetExperienceCameraMode() const
{
	if (ExperienceComponent)
	{
		return ExperienceComponent->GetCurrentExperienceCameraMode();
	}
	return nullptr;
}

bool AAssassinsGameState::IsChampionAvailable(const UAssassinsPawnData* InPawnData) const
{
	return InPawnData == nullptr ? false : !SelectedChampions.Contains(InPawnData->GetPrimaryAssetId());
}

void AAssassinsGameState::Multicast_ConstructChampionSelectionInfoFromPlayers_Implementation()
{
	for (APlayerState* PS : PlayerArray)
	{
		if (PS->IsInactive()) 
		{
			continue;
		}

		if (AAssassinsPlayerState* AssassinsPS = Cast<AAssassinsPlayerState>(PS))
		{
			if (const UAssassinsPawnData* ChampionData = AssassinsPS->GetPawnData<UAssassinsPawnData>())
			{
				SelectedChampions.Add(ChampionData->GetPrimaryAssetId());
			}
		}
	}
}

void AAssassinsGameState::AddSelectedChampion(const UAssassinsPawnData* InPawnData)
{
	const UPrimaryDataAsset* PawnDataAsset = Cast<UPrimaryDataAsset>(InPawnData);
	check(PawnDataAsset);

	SelectedChampions.Add(PawnDataAsset->GetPrimaryAssetId());

	Multicast_OnChampionSelectionChanged(PawnDataAsset->GetPrimaryAssetId(), true);
}

void AAssassinsGameState::RemoveSelectedChampion(const UAssassinsPawnData* InPawnData)
{
	const UPrimaryDataAsset* PawnDataAsset = Cast<UPrimaryDataAsset>(InPawnData);
	check(PawnDataAsset);

	const FPrimaryAssetId PawnAssetId = PawnDataAsset->GetPrimaryAssetId();

	if (SelectedChampions.Contains(PawnAssetId))
	{
		SelectedChampions.Remove(PawnAssetId);

		Multicast_OnChampionSelectionChanged(PawnAssetId, false);
	}
}

void AAssassinsGameState::Multicast_OnChampionSelectionChanged_Implementation(const FPrimaryAssetId& ChampionAssetId, bool bSelected)
{
	if (bSelected)
	{
		SelectedChampions.Add(ChampionAssetId);
	}
	else
	{
		SelectedChampions.Remove(ChampionAssetId);
	}

	if (UAssassinsFrontendStateComponent* FrontendComponent = FindComponentByClass<UAssassinsFrontendStateComponent>())
	{
		// Activate or deactivate the champion button.
		FrontendComponent->UpdateChampionSelectionScreen(ChampionAssetId, bSelected);
	}
}
