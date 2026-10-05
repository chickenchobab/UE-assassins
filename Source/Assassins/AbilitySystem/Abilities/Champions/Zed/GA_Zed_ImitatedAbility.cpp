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
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_SPAWNING_SHADOW, "Status.Champion.Zed.SpawningShadow");

	static constexpr EAbilityCustomReplicatedEvent ImitateEvent = EAbilityCustomReplicatedEvent::GameCustom1;
};

UGA_Zed_ImitatedAbility::UGA_Zed_ImitatedAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Zed_ImitatedAbility::BeginCombo()
{
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
	if (K2_HasAuthority() && !IsLocallyControlled())
	{
		UAbilityTask_WaitReplicatedEvent* ImitateTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, ZedImitation::ImitateEvent);
		ImitateTask->OnEvent.AddDynamic(this, &ThisClass::OnImitateRequested);
		ImitateTask->ReadyForActivation();
	}

	if (IsActive() && IsLocallyControlled())
	{
		UAbilityTask_WaitGameplayTagRemoved* SpawningTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
			this, ZedImitation::TAG_STATUS_SPAWNING_SHADOW, nullptr, true);
		SpawningTask->Removed.AddDynamic(this, &ThisClass::OnShadowSpawned);
		SpawningTask->ReadyForActivation();
	}
}

void UGA_Zed_ImitatedAbility::OnShadowSpawned()
{
	UAbilityTask_WaitGameplayTagRemoved* ComboTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, GetPartnerComboTag(), nullptr, true);
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
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Zed_ImitatedAbility::OnMontageComplete()
{
	IsMontageEnded = true;

	if (IsShadowHandled)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
