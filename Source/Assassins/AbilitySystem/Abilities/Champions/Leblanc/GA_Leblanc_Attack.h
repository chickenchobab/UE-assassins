// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Ranged.h"
#include "GA_Leblanc_Attack.generated.h"

/**
 * UGA_Leblanc_Attack
 *
 * Leblanc basic attack. Everything is the ranged attack, except that her projectile leaves from the socket on
 * her hand. Was the GA_Leblanc_Attack blueprint.
 */
UCLASS(Abstract)
class UGA_Leblanc_Attack : public UGA_Attack_Ranged
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~IAssassinsProjectileHandler interface
	virtual void SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform) override;
	//~End of IAssassinsProjectileHandler interface
};
