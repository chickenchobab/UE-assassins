#pragma once

#include "AbilitySystem/Abilities/Death/GA_Death.h"
#include "GA_Nexus_Death.generated.h"

UCLASS(Abstract)
class UGA_Nexus_Death : public UGA_Death
{
	GENERATED_BODY()

protected:

	virtual void OnMontageEnd() override;
};
