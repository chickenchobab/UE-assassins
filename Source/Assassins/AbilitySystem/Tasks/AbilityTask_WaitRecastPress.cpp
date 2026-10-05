// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Tasks/AbilityTask_WaitRecastPress.h"

#include "AbilitySystemComponent.h"

UAbilityTask_WaitRecastPress* UAbilityTask_WaitRecastPress::WaitRecastPress(UGameplayAbility* OwningAbility, FGameplayTagContainer BlockingTags, bool bTestAlreadyPressed)
{
	UAbilityTask_WaitRecastPress* Task = NewAbilityTask<UAbilityTask_WaitRecastPress>(OwningAbility);
	Task->BlockingTags = BlockingTags;
	Task->bTestInitialState = bTestAlreadyPressed;
	return Task;
}

void UAbilityTask_WaitRecastPress::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if ((ASC == nullptr) || (Ability == nullptr))
	{
		EndTask();
		return;
	}

	// The server of a remote client takes the presses its client sends: the client already turned down those it had to.
	// A press that came before the task started counts.
	if (!IsLocallyControlled())
	{
		StartListening();
		if (IsForRemoteClient() && !ASC->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()))
		{
			SetWaitingOnRemotePlayerData();
		}
		return;
	}

	for (const FGameplayTag& Tag : BlockingTags)
	{
		const FDelegateHandle Handle = ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnBlockingTagChanged);
		BlockingTagDelegateHandles.Emplace(Tag, Handle);
	}

	if (IsBlocked())
	{
		return;
	}

	if (bTestInitialState)
	{
		const FGameplayAbilitySpec* Spec = Ability->GetCurrentAbilitySpec();
		if (Spec && Spec->InputPressed)
		{
			OnPressCallback();
			return;
		}
	}

	StartListening();
}

void UAbilityTask_WaitRecastPress::OnBlockingTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// A press made meanwhile was turned down: the input may still be down from it, and only the next press counts.
	if (IsBlocked())
	{
		StopListening();
	}
	else
	{
		StartListening();
	}
}

void UAbilityTask_WaitRecastPress::OnPressCallback()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if ((ASC == nullptr) || (Ability == nullptr))
	{
		return;
	}

	StopListening();

	// As UAbilityTask_WaitInputPress: the client tells the server in a prediction window of its own, and the server
	// handles the press in a window of the same key.
	FScopedPredictionWindow ScopedPrediction(ASC, IsPredictingClient());

	if (IsPredictingClient())
	{
		ASC->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey(), ASC->ScopedPredictionKey);
	}
	else
	{
		ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey());
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnPress.Broadcast();
	}
	EndTask();
}

bool UAbilityTask_WaitRecastPress::IsBlocked() const
{
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	return ASC && ASC->HasAnyMatchingGameplayTags(BlockingTags);
}

void UAbilityTask_WaitRecastPress::StartListening()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if ((ASC == nullptr) || PressDelegateHandle.IsValid())
	{
		return;
	}

	PressDelegateHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()).AddUObject(this, &ThisClass::OnPressCallback);
}

void UAbilityTask_WaitRecastPress::StopListening()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC && PressDelegateHandle.IsValid())
	{
		ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(PressDelegateHandle);
	}
	PressDelegateHandle.Reset();
}

void UAbilityTask_WaitRecastPress::OnDestroy(bool bInOwnerFinished)
{
	StopListening();

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& TagHandle : BlockingTagDelegateHandles)
		{
			ASC->UnregisterGameplayTagEvent(TagHandle.Value, TagHandle.Key, EGameplayTagEventType::NewOrRemoved);
		}
	}
	BlockingTagDelegateHandles.Empty();

	Super::OnDestroy(bInOwnerFinished);
}
