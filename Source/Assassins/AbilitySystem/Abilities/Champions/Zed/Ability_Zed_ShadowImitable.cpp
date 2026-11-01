#include "AbilitySystem/Abilities/Champions/Zed/Ability_Zed_ShadowImitable.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AbilitySystemComponent.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Zed/AssassinsChampionSkillState_Zed.h"
#include "Character/Champions/Zed/ZedShadow.h"
#include "NativeGameplayTags.h"

namespace ZedImitation
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_SPAWNING_SHADOW, "Status.Champion.Zed.SpawningShadow");

	static constexpr EAbilityCustomReplicatedEvent ImitateEvent = EAbilityCustomReplicatedEvent::GameCustom1;
};

UAbility_Zed_ShadowImitable::UAbility_Zed_ShadowImitable(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbility_Zed_ShadowImitable::BeginCombo()
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

void UAbility_Zed_ShadowImitable::WaitToImitate()
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

void UAbility_Zed_ShadowImitable::OnShadowSpawned()
{
	UAbilityTask_WaitGameplayTagRemoved* ComboTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, GetPartnerComboTag(), nullptr, true);
	ComboTask->Removed.AddDynamic(this, &ThisClass::OnComboPartnerDone);
	ComboTask->ReadyForActivation();
}

void UAbility_Zed_ShadowImitable::OnComboPartnerDone()
{
	SendPredictedEventToServer(ZedImitation::ImitateEvent, [this]() { MakeShadowsImitate(); });

	RemoveTagFromAvatar(GetComboTag());
	HandleShadowsDone();
}

void UAbility_Zed_ShadowImitable::OnImitateRequested()
{
	MakeShadowsImitate();
	HandleShadowsDone();
}

void UAbility_Zed_ShadowImitable::MakeShadowsImitate()
{
	const UAssassinsChampionSkillState_Zed* ZedState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Zed>(GetAvatarActorFromActorInfo());
	if (ZedState == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<AZedShadow>> Shadows = ZedState->GetShadows();
	for (AZedShadow* Shadow : Shadows)
	{
		if (IsValid(Shadow))
		{
			ImitateWith(*Shadow);
		}
	}
}

void UAbility_Zed_ShadowImitable::HandleShadowsDone()
{
	IsShadowHandled = true;

	if (IsMontageEnded)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UAbility_Zed_ShadowImitable::OnMontageComplete()
{
	IsMontageEnded = true;

	if (IsShadowHandled)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UAbility_Zed_ShadowImitable::OnMontageCancelled()
{
	OnMontageComplete();
}

void UAbility_Zed_ShadowImitable::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	IsShadowHandled = false;
	IsMontageEnded = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
