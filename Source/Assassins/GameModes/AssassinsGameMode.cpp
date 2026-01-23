// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssassinsGameMode.h"
#include "Player/AssassinsPlayerController.h"
#include "Player/AssassinsPlayerSpawnerComponent.h"
#include "Player/AssassinsPlayerState.h"
#include "Player/AssassinsPlayerStart.h"
#include "GameModes/AssassinsExperienceDefinition.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "GameModes/AssassinsWorldSettings.h"
#include "GameModes/AssassinsGameState.h"
#include "Character/AssassinsCharacter.h"
#include "Character/AssassinsPawnData.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "AssassinsLogCategories.h"
#include "System/AssassinsAssetManager.h"
#include "UI/AssassinsHUD.h"
#include "Bot/AssassinsPlayerBotController.h"


AAssassinsGameMode::AAssassinsGameMode()
{
	GameStateClass = AAssassinsGameState::StaticClass();
	PlayerControllerClass = AAssassinsPlayerController::StaticClass();
	PlayerStateClass = AAssassinsPlayerState::StaticClass();
	DefaultPawnClass = AAssassinsCharacter::StaticClass();
	HUDClass = AAssassinsHUD::StaticClass();

	bUseSeamlessTravel = true;
}

const UAssassinsPawnData* AAssassinsGameMode::GetPawnDataForController(const AController* InController) const
{
	// See if pawn data is already set on the player state
	if (InController != nullptr)
	{
		if (const AAssassinsPlayerState* AssassinsPS = InController->GetPlayerState<AAssassinsPlayerState>())
		{
			if (const UAssassinsPawnData* PawnData = AssassinsPS->GetPawnData<UAssassinsPawnData>())
			{
				return PawnData;
			}
		}
	}

	// If not, fall back to the default for the current experience
	check(GameState);
	UAssassinsExperienceStateComponent* ExperienceComponent = GameState->FindComponentByClass<UAssassinsExperienceStateComponent>();
	check(ExperienceComponent);

	if (ExperienceComponent->IsExperienceLoaded())
	{
		const UAssassinsExperienceDefinition* Experience = ExperienceComponent->GetCurrentExperienceChecked();
		if (Experience->DefaultPawnData != nullptr)
		{
			return Experience->DefaultPawnData;
		}

		// Experience is loaded and there's still no pawn data, fall back to the default for now
		return UAssassinsAssetManager::Get().GetDefaultPawnData();
	}

	// Experience is not loaded yet, so there is no pawn data to be had
	return nullptr;
}

void AAssassinsGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Wait for the next frame to give time to initialize startup settings
	 GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleMatchAssignmentIfNotExpectingOne);
}

void AAssassinsGameMode::InitGameState()
{
	Super::InitGameState();

	// Listen for the experience load to complete
	UAssassinsExperienceStateComponent* ExperienceComponent = GameState->FindComponentByClass<UAssassinsExperienceStateComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnAssassinsExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

bool AAssassinsGameMode::UpdatePlayerStartSpot(AController* Player, const FString& Portal, FString& OutErrorMessage)
{
	// Me: Originally, this function is called by InitNewPlayer
	// Do nothing, we'll wait until PostLogin wehn try to spawn the player for real.
	// Doing anything right now is no good, systems like team assignment haven't even occurred yet.
	return true;
}

void AAssassinsGameMode::GenericPlayerInitialization(AController* NewPlayer)
{
	Super::GenericPlayerInitialization(NewPlayer);
}

void AAssassinsGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Delay starting new players until the experience has been loaded
	// (Players who log in prior to that will be started by OnExperienceLoaded)
	if (IsExperienceLoaded())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

bool AAssassinsGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return ControllerCanRestart(Player);
}

bool AAssassinsGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// We never want to user the start spot, alway use the player spawner component
	return false;
}

AActor* AAssassinsGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (UAssassinsPlayerSpawnerComponent* PlayerSpawnerComponent = GameState->FindComponentByClass<UAssassinsPlayerSpawnerComponent>())
	{
		return PlayerSpawnerComponent->ChoosePlayerStart(Player);
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AAssassinsGameMode::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	if (AAssassinsPlayerStart* AssassinsStart = Cast<AAssassinsPlayerStart>(StartSpot))
	{
		AssassinsStart->SetOccupied();
	}

	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
}

UClass* AAssassinsGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const UAssassinsPawnData* PawnData = GetPawnDataForController(InController))
	{
		if (PawnData->PawnClass)
		{
			return PawnData->PawnClass;
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* AAssassinsGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient; // Never save the default player pawns into a map.
	SpawnInfo.bDeferConstruction = true;

	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo))
		{
			if (UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(SpawnedPawn))
			{
				if (const UAssassinsPawnData* PawnData = GetPawnDataForController(NewPlayer))
				{
					PawnExtComp->SetPawnData(PawnData);
				}
				else
				{
					UE_LOG(LogAssassins, Error, TEXT("Game mode was unable to set PawnData on the spawned pawn [%s]"), *GetNameSafe(SpawnedPawn));
				}
			}

			SpawnedPawn->FinishSpawning(SpawnTransform);

			return SpawnedPawn;
		}
		else
		{
			UE_LOG(LogAssassins, Error, TEXT("Game mode was unable to spawn Pawn of class [%s] at [%s]."), *GetNameSafe(PawnClass), *SpawnTransform.ToHumanReadableString());
		}
	}

	return nullptr;
}

void AAssassinsGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (UAssassinsPlayerSpawnerComponent* PlayerSpawnerComponent = GameState->FindComponentByClass<UAssassinsPlayerSpawnerComponent>())
	{
		PlayerSpawnerComponent->FinishRestartPlayer(NewPlayer, StartRotation);
	}
	
	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

void AAssassinsGameMode::FailedToRestartPlayer(AController* NewPlayer)
{
	Super::FailedToRestartPlayer(NewPlayer); //Me: Just set state for failure

	// If we tried to spawn a pawn and if failed, lets try again
	// "note" check if there's actually a pawn class before we try this forever.
	if (UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer))
	{
		if (APlayerController* NewPC = Cast<APlayerController>(NewPlayer))
		{
			// If it's a player don't loop forever, maybe something changed and they can go longer restart if so stop trying.
			if (PlayerCanRestart(NewPC))
			{
				RequestPlayerRestartNextFrame(NewPlayer, false);
			}
			else
			{
				UE_LOG(LogAssassins, Verbose, TEXT("FailedToRestartPlayer(%s) and PlayerCanRestart returned false, so we're not going to try again."), *GetPathNameSafe(NewPlayer));
			}
		}
		else
		{
			RequestPlayerRestartNextFrame(NewPlayer, false);
		}
	}
	else
	{
		UE_LOG(LogAssassins, Verbose, TEXT("FailedToRestartPlayer(%s) but there's no pawn class so giving up."), *GetPathNameSafe(NewPlayer));
	}
	
}

void AAssassinsGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (bForceReset && (Controller != nullptr))
	{
		Controller->Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PC, &APlayerController::ServerRestartPlayer_Implementation);
	}
	else if (AAssassinsPlayerBotController* BotController = Cast<AAssassinsPlayerBotController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(BotController, &AAssassinsPlayerBotController::ServerRestartController);
	}
}

bool AAssassinsGameMode::ControllerCanRestart(AController* Controller)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (!Super::PlayerCanRestart_Implementation(PC))
		{
			return false;
		}
	}
	else
	{
		// Bot version of Super::PlayerCanRestart_Implementation
		if ((Controller == nullptr) || (Controller->IsPendingKillPending()))
		{
			return false;
		}
	}
	
	if (UAssassinsPlayerSpawnerComponent* PlayerSpawnerComponent = GameState->FindComponentByClass<UAssassinsPlayerSpawnerComponent>())
	{
		return PlayerSpawnerComponent->ControllerCanRestart(Controller);
	}

	return true;
}

bool AAssassinsGameMode::IsExperienceLoaded() const
{
	check(GameState);
	UAssassinsExperienceStateComponent* ExperienceComponent = GameState->FindComponentByClass<UAssassinsExperienceStateComponent>();
	check(ExperienceComponent);

	return ExperienceComponent->IsExperienceLoaded();
}

void AAssassinsGameMode::HandleMatchAssignmentIfNotExpectingOne()
{
	FPrimaryAssetId ExperienceId;
	FString ExperienceIdSource;

	// Precedence order (highest wins)
	//  - Matchmaking assignment (if present)
	//  - URL Options override
	//  - World Settings
	//  - TODO: Dedicated server
	//  - Default experience

	UWorld* World = GetWorld();

	if (!ExperienceId.IsValid() && UGameplayStatics::HasOption(OptionsString, TEXT("Experience")))
	{
		//Me: Get the name of the experience set by experience entry
		const FString ExperienceFromOptions = UGameplayStatics::ParseOption(OptionsString, TEXT("Experience"));
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType(UAssassinsExperienceDefinition::StaticClass()->GetFName()), FName(*ExperienceFromOptions));
		ExperienceIdSource = TEXT("OptionsString");
	}

	// See if the world settings has a default experience
	if (!ExperienceId.IsValid())
	{
		if (AAssassinsWorldSettings* TypedWorldSettings = Cast<AAssassinsWorldSettings>(GetWorldSettings()))
		{
			ExperienceId = TypedWorldSettings->GetDefaultGameplayExperience();
			ExperienceIdSource = TEXT("WorldSettings");
		}
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	FAssetData Dummy;
	if (ExperienceId.IsValid() && !AssetManager.GetPrimaryAssetData(ExperienceId, /*out*/ Dummy))
	{
		UE_LOG(LogAssassinsExperience, Error, TEXT("EXPERIENCE: Wanted to use %s but couldn't find it, falling back to the default)"), *ExperienceId.ToString());
		ExperienceId = FPrimaryAssetId();
	}

	// Final fallback to the default experience
	if (!ExperienceId.IsValid())
	{
		//TODO: TryDedicatedServerLogin

		//@TODO: Pull this from a config setting or something
		ExperienceId = FPrimaryAssetId(FPrimaryAssetType("AssassinsExperienceDefinition"), FName("B_AssassinsDefaultExperience"));
		ExperienceIdSource = TEXT("Default");
	}

	OnMatchAssignmentGiven(ExperienceId, ExperienceIdSource);
}

void AAssassinsGameMode::OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource)
{
	if (ExperienceId.IsValid())
	{
		UE_LOG(LogAssassinsExperience, Log, TEXT("Identified experience %s (Source: %s)"), *ExperienceId.ToString(), *ExperienceIdSource);

		UAssassinsExperienceStateComponent* ExperienceComponent = GameState->FindComponentByClass<UAssassinsExperienceStateComponent>();
		check(ExperienceComponent);
		ExperienceComponent->SetCurrentExperience(ExperienceId);
	}
	else
	{
		UE_LOG(LogAssassinsExperience, Error, TEXT("Failed to identify experience, loading screen will stay up forever"));
	}
}

void AAssassinsGameMode::OnExperienceLoaded(const UAssassinsExperienceDefinition* CurrentExperience)
{
	// Spawn any players that are already attached
	//@TODO: Here we're handling only *player* controllers, but in GetDefaultPawnClassForController_Implementation all controllers are skipped
	// GetDefaultPawnClassForController_Implementation might only be getting called for players anyways
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);
		if ((PC != nullptr) && (PC->GetPawn() == nullptr))
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}