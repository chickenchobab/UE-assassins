// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/BotCreation/AssassinsPlayerBotCreationComponent.h"
#include "GameModes/AssassinsGameMode.h"
#include "Character/AssassinsCharacter.h"
#include "Character/AssassinsPawnData.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Player/AssassinsPlayerState.h"
#include "AIController.h"

#if WITH_SERVER_CODE

void UAssassinsPlayerBotCreationComponent::ServerCreateBots()
{
	for (const auto& KVP : BotsToCreate)
	{
		SpawnOneBot(KVP.Key, KVP.Value);
	}
}

void UAssassinsPlayerBotCreationComponent::SpawnOneBot(UAssassinsPawnData* PawnData, int32 TeamId)
{
	check(PawnData);
	
	TSubclassOf<APawn> PawnClass = PawnData->PawnClass;
	check(PawnClass);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = GetComponentLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;

	APawn* DefaultPawn = PawnData->PawnClass->GetDefaultObject<APawn>();
	check(DefaultPawn);
	//Me: bWantsPlayerState should be set
	TSubclassOf<AController> ControllerClass = DefaultPawn->AIControllerClass;

	AAIController* NewController = GetWorld()->SpawnActor<AAIController>(ControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	
	if (NewController != nullptr)
	{
		AAssassinsGameMode* GameMode = GetGameMode<AAssassinsGameMode>();
		check(GameMode);

		if (AAssassinsPlayerState* AssassinsPS = Cast<AAssassinsPlayerState>(NewController->PlayerState))
		{
			AssassinsPS->SetGenericTeamId(IntegerToGenericTeamId(TeamId));
			AssassinsPS->SetPawnData(PawnData);
		}

		GameMode->GenericPlayerInitialization(NewController);
		GameMode->RestartPlayer(NewController);

		if (NewController->GetPawn() != nullptr)
		{
			if (UAssassinsPawnExtensionComponent* PawnExtComponent = NewController->GetPawn()->FindComponentByClass<UAssassinsPawnExtensionComponent>())
			{
				PawnExtComponent->CheckDefaultInitialization();
			}
		}

		SpawnedBotList.Add(NewController);
	}
}

#endif