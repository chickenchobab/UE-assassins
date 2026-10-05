// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"

#include "AbilitySystem/AssassinsProjectile.h"
#include "Character/AssassinsChampion.h"
#include "Components/CapsuleComponent.h"

namespace AkaliUltimate
{
	// The size of the capsule, as the UltimateCapsuleCollision of B_Akali had it. It stands on Akali's root.
	static constexpr float CapsuleRadius = 88.0f;
	static constexpr float CapsuleHalfHeight = 96.0f;
};

void UAssassinsChampionSkillState_Akali::Initialize()
{
	AAssassinsChampion* Champion = GetChampion();
	if (Champion == nullptr)
	{
		return;
	}

	// Each machine makes its own, as the skill state is: the server finds the hits with it, the owning client sees them
	// coming. It overlaps the pawns only, and only while an ability holds it(AcquireUltimateCapsule).
	UltimateCapsule = NewObject<UCapsuleComponent>(Champion, TEXT("UltimateCapsule"));
	UltimateCapsule->InitCapsuleSize(AkaliUltimate::CapsuleRadius, AkaliUltimate::CapsuleHalfHeight);
	UltimateCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UltimateCapsule->SetCollisionObjectType(ECC_WorldDynamic);
	UltimateCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	UltimateCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	UltimateCapsule->SetGenerateOverlapEvents(true);
	UltimateCapsule->SetCanEverAffectNavigation(false);
	UltimateCapsule->SetHiddenInGame(true);
	UltimateCapsule->SetupAttachment(Champion->GetRootComponent());
	UltimateCapsule->RegisterComponent();
}

UCapsuleComponent* UAssassinsChampionSkillState_Akali::AcquireUltimateCapsule(const UObject* User)
{
	if (UltimateCapsule == nullptr)
	{
		return nullptr;
	}

	UltimateCapsuleUsers.Add(User);

	UltimateCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UltimateCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	return UltimateCapsule;
}

void UAssassinsChampionSkillState_Akali::ReleaseUltimateCapsule(const UObject* User)
{
	UltimateCapsuleUsers.Remove(User);

	// Users that went away without letting go do not keep the collision on.
	for (auto It = UltimateCapsuleUsers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (UltimateCapsuleUsers.IsEmpty() && UltimateCapsule)
	{
		UltimateCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
