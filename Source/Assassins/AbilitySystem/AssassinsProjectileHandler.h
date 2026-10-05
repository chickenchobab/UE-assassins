// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Interface.h"
#include "AssassinsProjectileHandler.generated.h"

class AAssassinsProjectile;

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

	// Native events: a native ability implements them here, the blueprints keep overriding them as events.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Assassins|Projectile")
	void SetProjectileClass();
	virtual void SetProjectileClass_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Assassins|Projectile")
	void SetProjectileSpawnTransform(AActor* SourceActor, FTransform& SpawnTransform);
	virtual void SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform) {}

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Assassins|Projectile")
	void HandleProjectile(AAssassinsProjectile* SpawnedProjectile);
	virtual void HandleProjectile_Implementation(AAssassinsProjectile* SpawnedProjectile) {}
};
