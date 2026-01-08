// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Interface.h"
#include "AssassinsProjectileHandler.generated.h"

UINTERFACE(MinimalAPI)
class UAssassinsProjectileHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * Abilities are classified by their targeting method.
 * This interface defines abilities that use projectiles.
 */
class ASSASSINS_API IAssassinsProjectileHandler
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Projectile")
	void SetProjectileClass();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Projectile")
	void SetProjectileSpawnTransform();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Projectile")
	void HandleProjectile(AAssassinsProjectile* SpawnedProjectile);
};
