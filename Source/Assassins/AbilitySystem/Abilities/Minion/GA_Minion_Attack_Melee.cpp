// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Minion/GA_Minion_Attack_Melee.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/Abilities/Minion/MinionAttackSupport.h"
#include "Character/AssassinsMinion.h"

void UGA_Minion_Attack_Melee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// How close the minion gets before it swings is the minion's own reach.
	const AAssassinsMinion* MinionAvatar = MinionAttackSupport::GetMinionAvatar(this);
	if (MinionAvatar == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	AbilityRange = MinionAvatar->AttackRange;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Minion_Attack_Melee::SetMontageToPlay()
{
	if (const UAssassinsMontageWithTiming* MontageData = MinionAttackSupport::PickRandomMontageData(MinionAttackSupport::GetMinionAvatar(this)))
	{
		MontageToPlay = MontageData->Montage;
		HitEventTime = MontageData->Timing;

		// Fitted to the attack speed, as the attacks the parent picks are.
		AdjustMontageRateAndEventTime();
		return;
	}

	Super::SetMontageToPlay();
}
