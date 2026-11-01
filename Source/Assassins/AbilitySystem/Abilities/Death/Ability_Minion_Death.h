#pragma once

#include "AbilitySystem/Abilities/Death/Ability_Death.h"
#include "Ability_Minion_Death.generated.h"

UCLASS(Abstract)
class UAbility_Minion_Death : public UAbility_Death
{
	GENERATED_BODY()

protected:

	virtual void PlayDeath() override;

private:

	UFUNCTION()
	void OnAttackingRemoved();

	UFUNCTION()
	void OnActivatingAttackRemoved();
};
