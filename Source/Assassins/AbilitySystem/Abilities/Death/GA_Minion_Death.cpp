// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Death/GA_Minion_Death.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "NativeGameplayTags.h"

namespace MinionDeathTags
{
	// Set while the attack ability runs.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ATTACKING, "Status.Ability.Attacking");

	// Set while the ability that keeps attacking runs.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ACTIVATING_ATTACK, "Status.Ability.ActivatingAttack");
};

void UGA_Minion_Death::PlayDeath()
{
	// Both are gone at once when the death cancelled the attack: the tasks answer right away then.
	UAbilityTask_WaitGameplayTagRemoved* WaitTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, MinionDeathTags::TAG_STATUS_ABILITY_ATTACKING, /*InOptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true);
	WaitTask->Removed.AddDynamic(this, &ThisClass::OnAttackingRemoved);
	WaitTask->ReadyForActivation();
}

void UGA_Minion_Death::OnAttackingRemoved()
{
	UAbilityTask_WaitGameplayTagRemoved* WaitTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, MinionDeathTags::TAG_STATUS_ABILITY_ACTIVATING_ATTACK, /*InOptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true);
	WaitTask->Removed.AddDynamic(this, &ThisClass::OnActivatingAttackRemoved);
	WaitTask->ReadyForActivation();
}

void UGA_Minion_Death::OnActivatingAttackRemoved()
{
	Super::PlayDeath();
}
