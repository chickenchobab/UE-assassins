// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/BotCreation/AssassinsMinionCreationComponent.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "GameModes/AssassinsGameMode.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/AssassinsCharacterWithAbilities.h"
#include "Teams/AssassinsTeamSubsystem.h"
#include "Teams/AssassinsTeamInfo.h"
#include "Teams/AssassinsTeamAsset.h"

UAssassinsMinionCreationComponent::UAssassinsMinionCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	MinionSpawnState = EAssassinsMinionWaveSpawningState::WaitingSpawnNewWave;

	MinionSpawnCount = 0;

	CurrentTeamIndex = 0;
	CurrentMinionTypeIndex = 0;
}

#if WITH_SERVER_CODE

void UAssassinsMinionCreationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MinionSpawnState == EAssassinsMinionWaveSpawningState::SpawningMinion && MinionWaveSize > 0)
	{
		const UAssassinsTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UAssassinsTeamSubsystem>();
		check(TeamSubsystem);

		int32 NumTeams = TeamSubsystem->GetNumTeams();
		for (int TeamIndex = 0; TeamIndex < NumTeams; ++TeamIndex)
		{
			SpawnOneBot();
			ChangeTeam();
		}

		++MinionSpawnCount;

		if (MinionSpawnCount % NumNormalMinions == 0)
		{
			ChangeMinionType();
		}
		
		if (MinionSpawnCount == MinionWaveSize)
		{
			if (MinionSpawnCount % NumNormalMinions != 0)
			{
				ChangeMinionType();
			}
			MinionSpawnCount = 0;

			MinionSpawnState = EAssassinsMinionWaveSpawningState::WaitingSpawnNewWave;

			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(
				TimerHandle,
				FTimerDelegate::CreateLambda([this]()
					{
						MinionSpawnState = EAssassinsMinionWaveSpawningState::SpawningMinion;
					}),
				MinionWaveTerm,
				false
			);
		}
		else
		{
			MinionSpawnState = EAssassinsMinionWaveSpawningState::DelayBetweenMinion;

			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(
				TimerHandle,
				FTimerDelegate::CreateLambda([this]()
					{
						MinionSpawnState = EAssassinsMinionWaveSpawningState::SpawningMinion;
					}),
				SpawnDelay,
				false
			);
		}
	}
}

void UAssassinsMinionCreationComponent::ServerCreateBots_Implementation()
{
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle, 
		FTimerDelegate::CreateLambda([this]()
		{ 
			MinionSpawnState = EAssassinsMinionWaveSpawningState::SpawningMinion; 
		}), 
		InitialSpawnDelay, 
		false
	);

	const UAssassinsTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UAssassinsTeamSubsystem>();
	if (ensure(TeamSubsystem))
	{
		TeamSubsystem->GetMinionInfo(MinionClass, MinionSpawnTransform, CurrentTeamIndex, CurrentMinionTypeIndex, false, false);
		MinionTeamId = TeamSubsystem->GetTeamOfIndex(CurrentTeamIndex);
	}
}

void UAssassinsMinionCreationComponent::SpawnOneBot()
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnInfo.OverrideLevel = GetComponentLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;

	AAssassinsCharacterWithAbilities* Minion = GetWorld()->SpawnActor<AAssassinsCharacterWithAbilities>(MinionClass, MinionSpawnTransform, SpawnInfo);
	if (Minion)
	{
		// Auto Possess AI setting of the minion should be set to 'Placed In World Or Spawned'
		if (IAssassinsTeamAgentInterface* ControllerWithTeam = Cast<IAssassinsTeamAgentInterface>(Minion->GetController()))
		{
			ControllerWithTeam->SetGenericTeamId(IntegerToGenericTeamId(MinionTeamId));
		}
		SetBlackBoardValues(Minion->GetController());
	}
}

void UAssassinsMinionCreationComponent::ChangeTeam_Implementation()
{
	const UAssassinsTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UAssassinsTeamSubsystem>();
	if (ensure(TeamSubsystem))
	{
		TeamSubsystem->GetMinionInfo(MinionClass, MinionSpawnTransform, CurrentTeamIndex, CurrentMinionTypeIndex, true, false);
		MinionTeamId = TeamSubsystem->GetTeamOfIndex(CurrentTeamIndex);
	}
}

void UAssassinsMinionCreationComponent::ChangeMinionType_Implementation()
{
	const UAssassinsTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UAssassinsTeamSubsystem>();
	if (ensure(TeamSubsystem))
	{
		TeamSubsystem->GetMinionInfo(MinionClass, MinionSpawnTransform, CurrentTeamIndex, CurrentMinionTypeIndex, false, true);
	}
}

#endif
