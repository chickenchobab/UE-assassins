// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AssassinsPlayerSpawnerComponent.h"
#include "Player/AssassinsPlayerStart.h"
//#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/PlayerStartPIE.h"


UAssassinsPlayerSpawnerComponent::UAssassinsPlayerSpawnerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRegister = true;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAssassinsPlayerSpawnerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ThisClass::OnLevelLoaded);

	UWorld* World = GetWorld();
	World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleOnActorSpawned));

	for (TActorIterator<AAssassinsPlayerStart> It(World); It; ++It)
	{
		if (AAssassinsPlayerStart* PlayerStart = *It)
		{
			CachedPlayerStarts.Add(PlayerStart);
		}
	}
}

bool UAssassinsPlayerSpawnerComponent::ControllerCanRestart(AController* Player)
{
	return true;
}

AActor* UAssassinsPlayerSpawnerComponent::ChoosePlayerStart(AController* Player)
{
	if (Player)
	{
#if WITH_EDITOR
		if (APlayerStart* PlayerStart = FindPlayFromHereStart(Player))
		{
			return PlayerStart;
		}
#endif

		TArray<AAssassinsPlayerStart*> StarterPoints;
		for (auto StartIt = CachedPlayerStarts.CreateIterator(); StartIt; ++StartIt)
		{
			if (AAssassinsPlayerStart* Start = (*StartIt).Get())
			{
				StarterPoints.Add(Start);
			}
			else
			{
				StartIt.RemoveCurrent();
			}
		}

		if (!StarterPoints.IsEmpty())
		{
			return StarterPoints[FMath::RandRange(0, StarterPoints.Num() - 1)];
		}
	}
	return nullptr;
}

void UAssassinsPlayerSpawnerComponent::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	OnFinishRestartPlayer(NewPlayer, StartRotation);
	K2_OnFinishRestartPlayer(NewPlayer, StartRotation);
}

void UAssassinsPlayerSpawnerComponent::OnLevelLoaded(ULevel* InLevel, UWorld* InWorld)
{
	if (InWorld == GetWorld())
	{
		for (AActor* Actor : InLevel->Actors)
		{
			if (AAssassinsPlayerStart* PlayerStart = Cast<AAssassinsPlayerStart>(Actor))
			{
				CachedPlayerStarts.AddUnique(PlayerStart);
			}
		}
	}
}

void UAssassinsPlayerSpawnerComponent::HandleOnActorSpawned(AActor* SpawnedActor)
{
	if (AAssassinsPlayerStart* PlayerStart = Cast<AAssassinsPlayerStart>(SpawnedActor))
	{
		CachedPlayerStarts.AddUnique(PlayerStart);
	}
}

#if WITH_EDITOR
APlayerStart* UAssassinsPlayerSpawnerComponent::FindPlayFromHereStart(AController* Player)
{
	// Only 'Play From Here' for a player controller, bots etc. should all spawn from normal spawn points.
	if (Player->IsA<APlayerController>())
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				if (APlayerStart* PlayerStart = *It)
				{
					if (PlayerStart->IsA<APlayerStartPIE>())
					{
						// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
						return PlayerStart;
					}
				}
			}
		}
	}

	return nullptr;
}
#endif
