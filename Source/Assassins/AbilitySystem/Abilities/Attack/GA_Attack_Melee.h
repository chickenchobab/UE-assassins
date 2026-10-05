// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack.h"
#include "GA_Attack_Melee.generated.h"

/**
 * UGA_Attack_Melee
 *
 * Attack that hits the target where it stands: the damage is applied the moment the montage lands.
 * Was the GA_Attack_Melee blueprint.
 */
UCLASS(Abstract)
class UGA_Attack_Melee : public UGA_Attack
{
	GENERATED_BODY()

protected:

	//~UGA_Attack interface
	virtual void OnAttackHit() override;
	//~End of UGA_Attack interface
};
