// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack.h"
#include "AbilitySystem/AssassinsProjectileHandler.h"
#include "GA_Attack_Ranged.generated.h"

class AAssassinsProjectile;

/**
 * UGA_Attack_Ranged
 *
 * Attack that throws a projectile at the target when the montage lands. The damage travels with the projectile,
 * so it is the projectile that applies it. Was the GA_Attack_Ranged blueprint.
 */
UCLASS(Abstract)
class UGA_Attack_Ranged : public UGA_Attack, public IAssassinsProjectileHandler
{
	GENERATED_BODY()

protected:

	//~UGA_Attack interface
	virtual void OnAttackHit() override;
	//~End of UGA_Attack interface

	// Spawns the projectile and hands it to HandleProjectile. Server only.
	void SpawnProjectile();

	//~IAssassinsProjectileHandler interface
	virtual void SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform) override;
	virtual void HandleProjectile_Implementation(AAssassinsProjectile* SpawnedProjectile) override;
	//~End of IAssassinsProjectileHandler interface

	// What the attack throws. The attacks deriving from this one pick it in SetProjectileClass.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	// How hard the projectile turns to stay on its target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double HomingAcceleration = 2500.0;

	// Where on the avatar the projectile leaves from. Which mesh the socket is looked up on is up to the attack.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName ProjectileSpawnSocket;
};
