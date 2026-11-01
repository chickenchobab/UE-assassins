#pragma once

#include "Character/AssassinsChampionSkillState.h"
#include "AssassinsChampionSkillState_Zed.generated.h"

class AZedShadow;

UCLASS()
class ASSASSINS_API UAssassinsChampionSkillState_Zed : public UAssassinsChampionSkillState
{
	GENERATED_BODY()

public:

	void AddShadow(AZedShadow* Shadow);
	void RemoveShadow(AZedShadow* Shadow);

	const TArray<TObjectPtr<AZedShadow>>& GetShadows() const { return Shadows; }

private:

	UPROPERTY()
	TArray<TObjectPtr<AZedShadow>> Shadows;
};
