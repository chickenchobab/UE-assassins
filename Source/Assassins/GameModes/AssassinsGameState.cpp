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

void AAssassinsGameState::OnRep_bInLobby()
{
	// Do what the frontend component might have omitted due to the late replication of bInLobby
	if (ExperienceComponent && ExperienceComponent->IsExperienceLoaded())
	{
		if (UAssassinsFrontendStateComponent* FrontendComponent = FindComponentByClass<UAssassinsFrontendStateComponent>())
		{
			FrontendComponent->CallOrRegister_ShowChampionSelectionScreen();
		}
	}
}