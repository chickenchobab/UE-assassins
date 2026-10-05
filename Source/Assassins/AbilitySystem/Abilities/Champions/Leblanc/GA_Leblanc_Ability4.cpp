#include "AbilitySystem/Abilities/Champions/Leblanc/GA_Leblanc_Ability4.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Leblanc/AssassinsChampionSkillState_Leblanc.h"
#include "NativeGameplayTags.h"

namespace LeblancMimicUltimate
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ABILITY4, "Status.Ability.Ability4");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ULTIMATE_RECAST, "Event.Champion.Leblanc.UltimateRecast");
};

UGA_Leblanc_Ability4::UGA_Leblanc_Ability4(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UGA_Leblanc_Ability4::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAssassinsChampionSkillState_Leblanc* LeblancState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Leblanc>(GetAvatarActorFromActorInfo());
	if ((TriggerEventData == nullptr) || (LeblancState == nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	SendGameplayEvent(LeblancState->GetMimicAbilityEventTag(), *TriggerEventData);

	UAbilityTask_WaitDelay* FrameTask = UAbilityTask_WaitDelay::WaitDelay(this, 0.0f);
	FrameTask->OnFinish.AddDynamic(this, &ThisClass::OnMimicStarted);
	FrameTask->ReadyForActivation();
}

void UGA_Leblanc_Ability4::OnMimicStarted()
{
	UAbilityTask_WaitGameplayTagRemoved* MimicTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, LeblancMimicUltimate::TAG_STATUS_ABILITY_ABILITY4, nullptr, true);
	MimicTask->Removed.AddDynamic(this, &ThisClass::OnMimicEnded);
	MimicTask->ReadyForActivation();

	if (!IsActive() || !IsLocallyControlled())
	{
		return;
	}

	WaitForRecast();
}

void UGA_Leblanc_Ability4::WaitForRecast()
{
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Leblanc_Ability4::OnRecastReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastPressed);
	PressTask->ReadyForActivation();
}

void UGA_Leblanc_Ability4::OnRecastPressed(float TimeWaited)
{
	SendGameplayEvent(LeblancMimicUltimate::TAG_EVENT_ULTIMATE_RECAST, FGameplayEventData());

	// 모방은 재시전을 처리할 수 있을 때만 받는다(W 모방은 착지 뒤). 그 전 입력은 사라지므로 모방이 이어지는 동안 모든 입력을 보낸다.
	// 모방이 재시전을 받은 뒤의 입력은 아무 데도 가지 않는다.
	if (IsActive())
	{
		WaitForRecast();
	}
}

void UGA_Leblanc_Ability4::OnMimicEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
