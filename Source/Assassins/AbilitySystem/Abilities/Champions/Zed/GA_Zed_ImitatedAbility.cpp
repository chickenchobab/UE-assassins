// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_ImitatedAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AbilitySystemComponent.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Zed/AssassinsChampionSkillState_Zed.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"
#include "NativeGameplayTags.h"

namespace ZedImitation
{
	// Set on the owning side while the shadow of Ability2 is on its way.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_SPAWNING_SHADOW, "Status.Champion.Zed.SpawningShadow");

	// The owning client tells the server the shadows imitate.
	static constexpr EAbilityCustomReplicatedEvent ImitateEvent = EAbilityCustomReplicatedEvent::GameCustom1;
};

UGA_Zed_ImitatedAbility::UGA_Zed_ImitatedAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Zed_ImitatedAbility::BeginCombo()
{
	// The owning side only: the server hears when the shadows go from the owning client.
	if (!IsLocallyControlled())
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && !ASC->HasMatchingGameplayTag(GetPartnerComboTag()))
	{
		AddTagToAvatar(GetComboTag());
	}
}

void UGA_Zed_ImitatedAbility::WaitToImitate()
{
	// The server has the shadows imitate when the owning client says they do.
	if (K2_HasAuthority() && !IsLocallyControlled())
	{
		UAbilityTask_WaitReplicatedEvent* ImitateTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, ZedImitation::ImitateEvent);
		ImitateTask->OnEvent.AddDynamic(this, &ThisClass::OnImitateRequested);
		ImitateTask->ReadyForActivation();
	}

	// The shadows imitate once they are all there. Right away when nothing is pending: the tag tasks fire at once.
	if (IsActive() && IsLocallyControlled())
	{
		UAbilityTask_WaitGameplayTagRemoved* SpawningTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
			this, ZedImitation::TAG_STATUS_SPAWNING_SHADOW, /*InOptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true);
		SpawningTask->Removed.AddDynamic(this, &ThisClass::OnShadowSpawned);
		SpawningTask->ReadyForActivation();
	}
}

void UGA_Zed_ImitatedAbility::OnShadowSpawned()
{
	UAbilityTask_WaitGameplayTagRemoved* ComboTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, GetPartnerComboTag(), /*InOptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true);
	ComboTask->Removed.AddDynamic(this, &ThisClass::OnComboPartnerDone);
	ComboTask->ReadyForActivation();
}

void UGA_Zed_ImitatedAbility::OnComboPartnerDone()
{
	SendPredictedEventToServer(ZedImitation::ImitateEvent, [this]() { MakeShadowsImitate(); });

	RemoveTagFromAvatar(GetComboTag());
	HandleShadowsDone();
}

void UGA_Zed_ImitatedAbility::OnImitateRequested()
{
	MakeShadowsImitate();
	HandleShadowsDone();
}

void UGA_Zed_ImitatedAbility::MakeShadowsImitate()
{
	const UAssassinsChampionSkillState_Zed* ZedState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Zed>(GetAvatarActorFromActorInfo());
	if (ZedState == nullptr)
	{
		return;
	}

	// A copy: what the shadows do must not change the list being walked.
	const TArray<TObjectPtr<AAssassinsZedShadow>> Shadows = ZedState->GetShadows();
	for (AAssassinsZedShadow* Shadow : Shadows)
	{
		if (IsValid(Shadow))
		{
			ImitateWith(*Shadow);
		}
	}
}

void UGA_Zed_ImitatedAbility::HandleShadowsDone()
{
	IsShadowHandled = true;

	if (IsMontageEnded)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
	}
}

void UGA_Zed_ImitatedAbility::OnMontageComplete()
{
	// The ability also waits for the shadows.
	IsMontageEnded = true;

	if (IsShadowHandled)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
	}
}

void UGA_Zed_ImitatedAbility::OnMontageCancelled()
{
	OnMontageComplete();
}

void UGA_Zed_ImitatedAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	IsShadowHandled = false;
	IsMontageEnded = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
