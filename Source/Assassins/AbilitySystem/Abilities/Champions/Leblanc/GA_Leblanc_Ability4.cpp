// Fill out your copyright notice in the Description page of Project Settings.

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
	// Owned by the mimic while it goes on.
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
	// The mimic is aimed with the event the ultimate came with. Without it, or without an ability to mimic, there is
	// nothing to do: not a cancellation, nothing was going on.
	UAssassinsChampionSkillState_Leblanc* LeblancState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Leblanc>(GetAvatarActorFromActorInfo());
	if ((TriggerEventData == nullptr) || (LeblancState == nullptr))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	SendGameplayEvent(LeblancState->GetMimicAbilityEventTag(), *TriggerEventData);

	// The tag of the mimic is watched from the next frame.
	UAbilityTask_WaitDelay* FrameTask = UAbilityTask_WaitDelay::WaitDelay(this, 0.0f);
	FrameTask->OnFinish.AddDynamic(this, &ThisClass::OnMimicStarted);
	FrameTask->ReadyForActivation();
}

void UGA_Leblanc_Ability4::OnMimicStarted()
{
	// Over with the mimic, or right away when no mimic started.
	UAbilityTask_WaitGameplayTagRemoved* MimicTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, LeblancMimicUltimate::TAG_STATUS_ABILITY_ABILITY4, /*InOptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true);
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
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/ true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Leblanc_Ability4::OnRecastReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, /*bTestAlreadyPressed*/ false);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastPressed);
	PressTask->ReadyForActivation();
}

void UGA_Leblanc_Ability4::OnRecastPressed(float TimeWaited)
{
	SendGameplayEvent(LeblancMimicUltimate::TAG_EVENT_ULTIMATE_RECAST, FGameplayEventData());

	// The mimic hears the recast only once it can act on it(the mimic of Ability2 once it landed): a press before then is
	// lost. So every press goes to the mimic for as long as it goes on, not only the first. One after the mimic took its
	// recast goes nowhere.
	if (IsActive())
	{
		WaitForRecast();
	}
}

void UGA_Leblanc_Ability4::OnMimicEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}
