#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack.h"
#include "GA_Attack_Melee.generated.h"

UCLASS(Abstract)
class UGA_Attack_Melee : public UGA_Attack
{
	GENERATED_BODY()

protected:

	virtual void OnAttackHit() override;
};
