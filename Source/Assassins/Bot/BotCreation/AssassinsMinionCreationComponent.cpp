// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/BotCreation/AssassinsMinionCreationComponent.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "GameModes/AssassinsGameMode.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/AssassinsCharacterWithAbilities.h"

UAssassinsMinionCreationComponent::UAssassinsMinionCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	MinionSpawnState = EAssassinsMinionWaveSpawningState::WaitingSpawnNewWave;

	MinionSpawnCount = 0;
}

#if WITH_SERVER_CODE

void UAssassinsMinionCreationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MinionSpawnState == EAssassinsMinionWaveSpawningState::SpawningMinion)
	{
		SpawnOneBot();
		ChangeTeam();
		SpawnOneBot();

		++MinionSpawnCount;

		if (MinionSpawnCount % NumNormalMinions == 0)
		{
			UpgradeMinion();
		}
		else if (MinionSpawnCount == MinionWaveSize)
		{
			UpgradeMinion();
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
}

void UAssassinsMinionCreationComponent::SpawnOneBot()
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnInfo.OverrideLevel = GetComponentLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;

	AAssassinsCharacterWithAbilities* Minion = GetWorld()->SpawnActor<AAssassinsCharacterWithAbilities>(MinionClass, MinionSpawnLocation, MinionSpawnRotation, SpawnInfo);
	if (Minion)
	{
		// Auto Possess AI setting of the minion should be set to 'Placed In World Or Spawned'
		if (IAssassinsTeamAgentInterface* ControllerWithTeam = Cast<IAssassinsTeamAgentInterface>(Minion->GetController()))
		{
			ControllerWithTeam->SetGenericTeamId(MinionTeamID);
		}
	}

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

#endif
