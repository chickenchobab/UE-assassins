#pragma once

#include "AbilitySystem/Abilities/Attack/Ability_Attack.h"
#include "Ability_MeleeAttack.generated.h"

UCLASS(Abstract)
class UAbility_MeleeAttack : public UAbility_Attack
{
	GENERATED_BODY()

protected:

	virtual void OnAttackHit() override;
};
