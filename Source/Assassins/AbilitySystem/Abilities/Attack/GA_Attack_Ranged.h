#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack.h"
#include "AbilitySystem/AssassinsProjectileHandler.h"
#include "GA_Attack_Ranged.generated.h"

class AAssassinsProjectile;

UCLASS(Abstract)
class UGA_Attack_Ranged : public UGA_Attack, public IAssassinsProjectileHandler
{
	GENERATED_BODY()

protected:

	virtual void OnAttackHit() override;

	void SpawnProjectile();

	virtual void SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform) override;
	virtual void HandleProjectile_Implementation(AAssassinsProjectile* SpawnedProjectile) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double HomingAcceleration = 2500.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName ProjectileSpawnSocket;
};
