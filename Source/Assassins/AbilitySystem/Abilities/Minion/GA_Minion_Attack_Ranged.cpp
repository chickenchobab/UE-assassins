// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Minion/GA_Minion_Attack_Ranged.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/Abilities/Minion/MinionAttackSupport.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "Bot/AssassinsBotController.h"
#include "Character/AssassinsMinion.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Particles/ParticleSystemComponent.h"
#include "Teams/AssassinsTeamAsset.h"
#include "Teams/AssassinsTeamInfo.h"
#include "Teams/AssassinsTeamSubsystem.h"

namespace MinionProjectile
{
	// The parameter the projectile material reads its color from.
	static const FName TeamColorParameterName(TEXT("TeamColor"));

	// A siege projectile is shown at its full size, the others are smaller.
	static constexpr double SiegeParticleScale = 1.0;
	static constexpr double RegularParticleScale = 0.7;
};

void UGA_Minion_Attack_Ranged::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// How close the minion gets before it shoots is the minion's own reach.
	const AAssassinsMinion* MinionAvatar = MinionAttackSupport::GetMinionAvatar(this);
	if (MinionAvatar == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	AbilityRange = MinionAvatar->AttackRange;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Minion_Attack_Ranged::SetMontageToPlay()
{
	if (const UAssassinsMontageWithTiming* MontageData = MinionAttackSupport::PickRandomMontageData(MinionAttackSupport::GetMinionAvatar(this)))
	{
		MontageToPlay = MontageData->Montage;
		HitEventTime = MontageData->Timing;

		// Fitted to the attack speed, as the attacks the parent picks are.
		AdjustMontageRateAndEventTime();
		return;
	}

	Super::SetMontageToPlay();
}

void UGA_Minion_Attack_Ranged::SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform)
{
	if (!IsValid(SourceActor))
	{
		return;
	}

	// The shot leaves from the socket on the minion mesh.
	if (const USkeletalMeshComponent* SourceMesh = SourceActor->GetComponentByClass<USkeletalMeshComponent>())
	{
		SpawnTransform = FTransform(SourceActor->GetActorRotation(), SourceMesh->GetSocketLocation(ProjectileSpawnSocket), FVector::OneVector);
		return;
	}

	Super::SetProjectileSpawnTransform_Implementation(SourceActor, SpawnTransform);
}

void UGA_Minion_Attack_Ranged::HandleProjectile_Implementation(AAssassinsProjectile* SpawnedProjectile)
{
	Super::HandleProjectile_Implementation(SpawnedProjectile);

	if (!IsValid(SpawnedProjectile))
	{
		return;
	}

	// The projectile carries the color of the team that shot it.
	if (UParticleSystemComponent* ProjectileParticle = SpawnedProjectile->GetProjectileParticle())
	{
		const FLinearColor TeamColor = GetTeamColorFromTeamInfo();
		ProjectileParticle->SetVectorParameter(MinionProjectile::TeamColorParameterName, FVector(TeamColor.R, TeamColor.G, TeamColor.B));

		const AAssassinsMinion* MinionAvatar = MinionAttackSupport::GetMinionAvatar(this);
		const double ParticleScale = (MinionAvatar && MinionAvatar->IsSiegeMinion) ? MinionProjectile::SiegeParticleScale : MinionProjectile::RegularParticleScale;
		ProjectileParticle->SetWorldScale3D(FVector(ParticleScale));
	}
}

FLinearColor UGA_Minion_Attack_Ranged::GetTeamColorFromTeamInfo() const
{
	const AAssassinsBotController* BotController = Cast<AAssassinsBotController>(GetControllerFromActorInfo());
	if (BotController == nullptr)
	{
		return FLinearColor::White;
	}

	const UWorld* World = GetWorld();
	const UAssassinsTeamSubsystem* TeamSubsystem = World ? World->GetSubsystem<UAssassinsTeamSubsystem>() : nullptr;
	if (TeamSubsystem == nullptr)
	{
		return FLinearColor::White;
	}

	const AAssassinsTeamInfo* TeamInfo = TeamSubsystem->GetTeamInfo(BotController->GetTeamId());
	if (!IsValid(TeamInfo))
	{
		return FLinearColor::White;
	}

	const UAssassinsTeamAsset* TeamAsset = TeamInfo->GetTeamAsset();
	if (!IsValid(TeamAsset))
	{
		return FLinearColor::White;
	}

	const FLinearColor* TeamColor = TeamAsset->ColorParameters.Find(MinionProjectile::TeamColorParameterName);
	return TeamColor ? *TeamColor : FLinearColor::White;
}
