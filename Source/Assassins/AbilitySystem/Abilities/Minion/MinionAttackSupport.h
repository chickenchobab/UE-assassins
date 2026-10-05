#pragma once

#include "CoreMinimal.h"

class AAssassinsMinion;
class UAssassinsMontageWithTiming;
class UGameplayAbility;

namespace MinionAttackSupport
{
	AAssassinsMinion* GetMinionAvatar(const UGameplayAbility* Ability);

	UAssassinsMontageWithTiming* PickRandomMontageData(const AAssassinsMinion* Minion);
}
