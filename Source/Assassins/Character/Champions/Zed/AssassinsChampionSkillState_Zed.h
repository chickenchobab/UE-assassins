#pragma once

#include "Character/AssassinsChampionSkillState.h"
#include "AssassinsChampionSkillState_Zed.generated.h"

class AAssassinsZedShadow;

UCLASS()
class ASSASSINS_API UAssassinsChampionSkillState_Zed : public UAssassinsChampionSkillState
{
	GENERATED_BODY()

public:

	void AddShadow(AAssassinsZedShadow* Shadow);
	void RemoveShadow(AAssassinsZedShadow* Shadow);

	const TArray<TObjectPtr<AAssassinsZedShadow>>& GetShadows() const { return Shadows; }

private:

	UPROPERTY()
	TArray<TObjectPtr<AAssassinsZedShadow>> Shadows;
};
