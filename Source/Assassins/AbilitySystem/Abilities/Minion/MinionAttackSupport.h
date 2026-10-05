// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AAssassinsMinion;
class UAssassinsMontageWithTiming;
class UGameplayAbility;

/**
 * The melee and the ranged minion attack both take their range and their montages from the minion they run on,
 * but they derive from different attacks, so the part they share lives here. Was the MinionAttackMacros library.
 */
namespace MinionAttackSupport
{
	AAssassinsMinion* GetMinionAvatar(const UGameplayAbility* Ability);

	// One of the minion montages, or null when it has none.
	UAssassinsMontageWithTiming* PickRandomMontageData(const AAssassinsMinion* Minion);
}
