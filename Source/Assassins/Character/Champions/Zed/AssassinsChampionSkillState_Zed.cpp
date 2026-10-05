#include "Character/Champions/Zed/AssassinsChampionSkillState_Zed.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"

namespace ZedShadows
{
	static constexpr int32 MaxShadows = 2;
};

void UAssassinsChampionSkillState_Zed::AddShadow(AAssassinsZedShadow* Shadow)
{
	Shadows.RemoveAll([](const TObjectPtr<AAssassinsZedShadow>& Existing) { return !IsValid(Existing); });

	if (IsValid(Shadow) && (Shadows.Num() < ZedShadows::MaxShadows))
	{
		Shadows.AddUnique(Shadow);
	}
}

void UAssassinsChampionSkillState_Zed::RemoveShadow(AAssassinsZedShadow* Shadow)
{
	Shadows.Remove(Shadow);
}
