#pragma once

#include "AbilitySystem/Abilities/Death/GA_Death.h"
#include "GA_Minion_Death.generated.h"

UCLASS(Abstract)
class UGA_Minion_Death : public UGA_Death
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
