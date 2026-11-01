#pragma once

#include "AbilitySystem/AssassinsInterframeHitCheckSubsystem.h"
#include "Character/AssassinsChampionSkillState.h"
#include "AssassinsChampionSkillState_Akali.generated.h"

class AAssassinsProjectile;

UCLASS()
class ASSASSINS_API UAssassinsChampionSkillState_Akali : public UAssassinsChampionSkillState
{
	GENERATED_BODY()

public:

	static FAssassinsInterframeHitShape GetUltimateHitShape();

	UPROPERTY()
	TObjectPtr<AAssassinsProjectile> SpawnedProjectile;
};
