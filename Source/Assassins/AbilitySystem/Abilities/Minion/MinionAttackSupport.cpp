// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Minion/MinionAttackSupport.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "Character/AssassinsMinion.h"

namespace MinionAttackSupport
{
	AAssassinsMinion* GetMinionAvatar(const UGameplayAbility* Ability)
	{
		return Ability ? Cast<AAssassinsMinion>(Ability->GetAvatarActorFromActorInfo()) : nullptr;
	}

	UAssassinsMontageWithTiming* PickRandomMontageData(const AAssassinsMinion* Minion)
	{
		if ((Minion == nullptr) || Minion->MontageDataArray.IsEmpty())
		{
			return nullptr;
		}

		return Minion->MontageDataArray[FMath::RandRange(0, Minion->MontageDataArray.Num() - 1)];
	}
}
