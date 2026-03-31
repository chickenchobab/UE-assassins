// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/BotCreation/AssassinsPlayerBotCreationComponent.h"
#include "GameModes/AssassinsGameMode.h"
#include "GameModes/AssassinsGameState.h"
#include "Character/AssassinsCharacter.h"
#include "Character/AssassinsPawnData.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Player/AssassinsPlayerState.h"
#include "AIController.h"
#include "Engine/AssetManager.h"

#if WITH_SERVER_CODE

void UAssassinsPlayerBotCreationComponent::ServerCreateBots()
{
	AAssassinsGameMode* GameMode = GetGameMode<AAssassinsGameMode>();
	check(GameMode);

	NumBotsToCreate = GameMode->GetNumBots();
	if (NumBotsToCreate > 0)
	{
		LoadChampionDataAndSpawnBots();
	}
}

void UAssassinsPlayerBotCreationComponent::SpawnOneBot(const UAssassinsPawnData* PawnData)
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
	TSubclassOf<AController> ControllerClass = DefaultPawn->AIControllerClass;

	AAIController* NewController = GetWorld()->SpawnActor<AAIController>(ControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	
	if (NewController != nullptr)
	{
		AAssassinsGameMode* GameMode = GetGameMode<AAssassinsGameMode>();
		check(GameMode);

		AAssassinsPlayerState* PS = Cast<AAssassinsPlayerState>(NewController->PlayerState);
		check(PS);
		PS->SetPawnData(PawnData);

		GameMode->GenericPlayerInitialization(NewController);
		GameMode->RestartPlayer(NewController);

		if (NewController->GetPawn() != nullptr)
		{
			if (UAssassinsPawnExtensionComponent* PawnExtComponent = NewController->GetPawn()->FindComponentByClass<UAssassinsPawnExtensionComponent>())
			{
				PawnExtComponent->CheckDefaultInitialization();
			}
		}

		AAssassinsGameState* GameState = GetGameStateChecked<AAssassinsGameState>();
		GameState->AddSelectedChampion(PawnData);

		SpawnedBotList.Add(NewController);
	}
}

void UAssassinsPlayerBotCreationComponent::LoadChampionDataAndSpawnBots()
{
	UAssetManager& AssetManager = UAssetManager::Get();

	FPrimaryAssetType AssetType(TEXT("AssassinsPawnData"));

	TArray<FPrimaryAssetId> AssetIds;
	AssetManager.GetPrimaryAssetIdList(AssetType, AssetIds);
	if (AssetIds.IsEmpty()) return;

	AssetPaths.Reserve(AssetIds.Num());
	for (const FPrimaryAssetId& Id : AssetIds)
	{
		FSoftObjectPath Path = AssetManager.GetPrimaryAssetPath(Id);
		if (Path.IsValid())
		{
			AssetPaths.Emplace(Path);
		}
	}

	ChampionDataList.Reserve(AssetPaths.Num());
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Streamable.RequestAsyncLoad(
		AssetPaths,
		FStreamableDelegate::CreateLambda([this]() {
			for (const FSoftObjectPath& Path : AssetPaths)
			{
				if (UAssassinsPawnData* ChampionData = Cast<UAssassinsPawnData>(Path.ResolveObject()))
				{
					ChampionDataList.Emplace(ChampionData);
				}
			}

			AAssassinsGameState* GameState = GetGameStateChecked<AAssassinsGameState>();
			for (int32 CurrentBot = 0; CurrentBot < NumBotsToCreate; ++CurrentBot)
			{
				for (const UAssassinsPawnData* PawnData : ChampionDataList)
				{
					if (GameState->IsChampionAvailable(PawnData))
					{
						SpawnOneBot(PawnData);
						break;
					}
				}
			}
		})
	);
}

#endif