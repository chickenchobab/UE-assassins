// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Champions/Zed/AssassinsZedShadow.h"

#include "Character/AssassinsChampion.h"
#include "Character/Champions/Zed/AssassinsChampionSkillState_Zed.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

AAssassinsZedShadow::AAssassinsZedShadow()
{
}

void AAssassinsZedShadow::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAssassinsZedShadow, ShadowLifeSpan);
}

AAssassinsZedShadow* AAssassinsZedShadow::SpawnShadow(TSubclassOf<AAssassinsZedShadow> ShadowClass, AActor* Zed, const FTransform& SpawnTransform, double LifeSpan)
{
	UWorld* World = Zed ? Zed->GetWorld() : nullptr;
	if ((World == nullptr) || !ShadowClass)
	{
		return nullptr;
	}

	// Deferred: the shadow starts the timer of its life as it begins play.
	AAssassinsZedShadow* Shadow = World->SpawnActorDeferred<AAssassinsZedShadow>(
		ShadowClass, SpawnTransform, Zed, /*Instigator*/ nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Shadow == nullptr)
	{
		return nullptr;
	}

	Shadow->ShadowLifeSpan = LifeSpan;
	Shadow->FinishSpawning(SpawnTransform);
	return Shadow;
}

void AAssassinsZedShadow::SwapPlacesWith(ACharacter* Zed)
{
	if (Zed == nullptr)
	{
		return;
	}

	const FVector ShadowLocation = GetActorLocation();
	const FRotator ShadowRotation = GetActorRotation();

	SetActorLocationAndRotation(Zed->GetActorLocation(), Zed->GetActorRotation(), /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);

	if (UAssassinsCharacterMovementComponent* Movement = Cast<UAssassinsCharacterMovementComponent>(Zed->GetCharacterMovement()))
	{
		Movement->TeleportCharacter(ShadowLocation, ShadowRotation);
	}
}

void AAssassinsZedShadow::RegisterWithOwner()
{
	if (UAssassinsChampionSkillState_Zed* ZedState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Zed>(GetOwner()))
	{
		ZedState->AddShadow(this);
	}
}

void AAssassinsZedShadow::UnregisterFromOwner()
{
	if (UAssassinsChampionSkillState_Zed* ZedState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Zed>(GetOwner()))
	{
		ZedState->RemoveShadow(this);
	}
}
