#pragma once

#include "UObject/Object.h"
#include "AssassinsChampionSkillState.generated.h"

class AAssassinsChampion;

UCLASS(Abstract)
class ASSASSINS_API UAssassinsChampionSkillState : public UObject
{
	GENERATED_BODY()

public:

	virtual void Initialize() {}

	AAssassinsChampion* GetChampion() const;
};
