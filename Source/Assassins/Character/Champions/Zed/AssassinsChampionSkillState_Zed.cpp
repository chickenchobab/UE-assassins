#include "Character/Champions/Zed/AssassinsChampionSkillState_Zed.h"
#include "Character/Champions/Zed/ZedShadow.h"

namespace ZedShadows
{
	static constexpr int32 MaxShadows = 2;
};

void UAssassinsChampionSkillState_Zed::AddShadow(AZedShadow* Shadow)
{
	Shadows.RemoveAll([](const TObjectPtr<AZedShadow>& Existing) { return !IsValid(Existing); });

	if (IsValid(Shadow) && (Shadows.Num() < ZedShadows::MaxShadows))
	{
		Shadows.AddUnique(Shadow);
	}
}

void UAssassinsChampionSkillState_Zed::RemoveShadow(AZedShadow* Shadow)
{
	Shadows.Remove(Shadow);
}
