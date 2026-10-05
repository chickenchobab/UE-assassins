// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Attack/GA_Attack_Ranged.h"

#include "AbilitySystem/AssassinsProjectile.h"

void UGA_Attack_Ranged::OnAttackHit()
{
	// Only the server throws, like the other abilities with a projectile: the projectile replicates, and the owning
	// client would otherwise see a copy of its own next to it.
	if (K2_HasAuthority())
	{
		SpawnProjectile();
	}

	// The next attack is asked for as soon as this one is thrown, without waiting for the projectile to land.
	SendActivateAttackEvent(AbilityTargetActor);

	Super::OnAttackHit();

	// The montage was already over when the projectile left, so the attack has nothing left to wait for.
	if (IsMontageEnded)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
	}
}

void UGA_Attack_Ranged::SpawnProjectile()
{
	// The attacks deriving from this one say what they throw.
	IAssassinsProjectileHandler::Execute_SetProjectileClass(this);

	FTransform SpawnTransform;
	IAssassinsProjectileHandler::Execute_SetProjectileSpawnTransform(this, GetAvatarActorFromActorInfo(), SpawnTransform);

	AAssassinsProjectile* SpawnedProjectile = SpawnAbilityProjectile(ProjectileClass, SpawnTransform);
	IAssassinsProjectileHandler::Execute_HandleProjectile(this, SpawnedProjectile);
}

void UGA_Attack_Ranged::SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform)
{
	// Thrown from where the avatar stands, unless an attack deriving from this one says otherwise.
	if (IsValid(SourceActor))
	{
		SpawnTransform = SourceActor->GetTransform();
	}
}

void UGA_Attack_Ranged::HandleProjectile_Implementation(AAssassinsProjectile* SpawnedProjectile)
{
	if (SpawnedProjectile == nullptr)
	{
		return;
	}

	// Nothing left to chase: the projectile goes away instead of flying at nobody.
	if (!IsValid(AbilityTargetActor))
	{
		SpawnedProjectile->Destroy();
		return;
	}

	// The damage travels with the projectile and is applied where it lands.
	SpawnedProjectile->InitHomingProjectile(AbilityTargetActor, MakeEffectSpecHandle(GetAttackEffectClass()), HomingAcceleration);
}
