#include "Character/AssassinsChampionSkillState.h"
#include "Character/AssassinsChampion.h"

AAssassinsChampion* UAssassinsChampionSkillState::GetChampion() const
{
	return GetTypedOuter<AAssassinsChampion>();
}
