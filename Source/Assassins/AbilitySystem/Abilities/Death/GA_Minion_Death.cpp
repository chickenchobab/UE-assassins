#include "AbilitySystem/Abilities/Death/GA_Minion_Death.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "NativeGameplayTags.h"

namespace MinionDeathTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ATTACKING, "Status.Ability.Attacking");

	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ACTIVATING_ATTACK, "Status.Ability.ActivatingAttack");
};

void UGA_Minion_Death::PlayDeath()
{
	UAbilityTask_WaitGameplayTagRemoved* WaitTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, MinionDeathTags::TAG_STATUS_ABILITY_ATTACKING, nullptr, true);
	WaitTask->Removed.AddDynamic(this, &ThisClass::OnAttackingRemoved);
	WaitTask->ReadyForActivation();
}

void UGA_Minion_Death::OnAttackingRemoved()
{
	UAbilityTask_WaitGameplayTagRemoved* WaitTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, MinionDeathTags::TAG_STATUS_ABILITY_ACTIVATING_ATTACK, nullptr, true);
	WaitTask->Removed.AddDynamic(this, &ThisClass::OnActivatingAttackRemoved);
	WaitTask->ReadyForActivation();
}

void UGA_Minion_Death::OnActivatingAttackRemoved()
{
	Super::PlayDeath();
}
