#pragma once

#include "AbilitySystem/Abilities/Death/Ability_Death.h"
#include "Ability_Nexus_Death.generated.h"

UCLASS(Abstract)
class UAbility_Nexus_Death : public UAbility_Death
{
	GENERATED_BODY()

protected:

	virtual void OnMontageEnd() override;
};
