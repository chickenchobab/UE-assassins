#include "AbilitySystem/Abilities/Minion/Ability_Minion_MeleeAttack.h"

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilitySystem/Abilities/Minion/MinionAttackSupport.h"
#include "Character/AssassinsMinion.h"

void UAbility_Minion_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const AAssassinsMinion* MinionAvatar = MinionAttackSupport::GetMinionAvatar(this);
	if (MinionAvatar == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AbilityRange = MinionAvatar->AttackRange;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAbility_Minion_MeleeAttack::SetMontageToPlay()
{
	if (const UAssassinsMontageWithTiming* MontageData = MinionAttackSupport::PickRandomMontageData(MinionAttackSupport::GetMinionAvatar(this)))
	{
		MontageToPlay = MontageData->Montage;
		HitEventTime = MontageData->Timing;

		AdjustMontageRateAndEventTime();
		return;
	}

	Super::SetMontageToPlay();
}
