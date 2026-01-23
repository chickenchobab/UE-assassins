// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AssassinsPlayerStart.h"
#include "GameFramework/GameModeBase.h"

AAssassinsPlayerStart::AAssassinsPlayerStart(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bOnePlayerPerSpot = true;
	Occupancy = EAssassinsPlayerStartOccupancy::Empty;
}

EAssassinsPlayerStartOccupancy AAssassinsPlayerStart::GetOccupancy(AController* const ControllerPawnToFit) const
{
	if (bOnePlayerPerSpot)
	{
		return Occupancy;
	}
	
	//Me: Code I tried out because it seemed interesting
	UWorld* const World = GetWorld();
	if (HasAuthority() && World)
	{
		if (AGameModeBase* AuthGameMode = World->GetAuthGameMode())
		{
			TSubclassOf<APawn> PawnClass = AuthGameMode->GetDefaultPawnClassForController(ControllerPawnToFit);
			const APawn* const PawnToFit = PawnClass ? GetDefault<APawn>(PawnClass) : nullptr;
			
			FVector ActorLocation = GetActorLocation();
			const FRotator ActorRotation = GetActorRotation();

			if (!World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation, nullptr))
			{
				return EAssassinsPlayerStartOccupancy::Empty;
			}
			else if (World->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
			{
				return EAssassinsPlayerStartOccupancy::Partial;
			}
		}
	}

	return EAssassinsPlayerStartOccupancy::Full;
}
