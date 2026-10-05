#include "AbilitySystem/Abilities/Attack/GA_ActivateAttack.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Character/AssassinsCharacter.h"
#include "NativeGameplayTags.h"
#include "Teams/AssassinsTeamBaseActor.h"

namespace ActivateAttack
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_DEATH, "Status.Death");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ATTACKING, "Status.Ability.Attacking");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ATTACK, "Event.Attack");
};

UGA_ActivateAttack::UGA_ActivateAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UGA_ActivateAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (TriggerEventData == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const AActor* TargetActor = TriggerEventData->Target;
	const AAssassinsCharacter* TargetCharacter = Cast<AAssassinsCharacter>(TargetActor);
	const bool bCanAttackTarget = TargetCharacter
		? !TargetCharacter->HasMatchingGameplayTag(ActivateAttack::TAG_STATUS_DEATH)
		: (TargetActor && TargetActor->IsA<AAssassinsTeamBaseActor>());

	if (!bCanAttackTarget)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AttackEventData = *TriggerEventData;

	UAbilityTask_WaitGameplayTagRemoved* AttackingTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, ActivateAttack::TAG_STATUS_ABILITY_ATTACKING, nullptr, true);
	AttackingTask->Removed.AddDynamic(this, &ThisClass::OnAttackingEnded);
	AttackingTask->ReadyForActivation();
}

void UGA_ActivateAttack::OnAttackingEnded()
{
	UAbilityTask_WaitDelay* FrameTask = UAbilityTask_WaitDelay::WaitDelay(this, 0.0f);
	FrameTask->OnFinish.AddDynamic(this, &ThisClass::OnAttackReady);
	FrameTask->ReadyForActivation();
}

void UGA_ActivateAttack::OnAttackReady()
{
	SendGameplayEvent(ActivateAttack::TAG_EVENT_ATTACK, AttackEventData);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_ActivateAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	AttackEventData = FGameplayEventData();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
