// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"

#include "AbilitySystemComponent.h"

UAbilityTask_WaitReplicatedEvent* UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(UGameplayAbility* OwningAbility, EAbilityCustomReplicatedEvent CustomEvent, bool bTriggerOnce)
{
	UAbilityTask_WaitReplicatedEvent* Task = NewAbilityTask<UAbilityTask_WaitReplicatedEvent>(OwningAbility);
	Task->EventType = UAssassinsGameplayAbility::ToGenericReplicatedEvent(CustomEvent);
	Task->bTriggerOnce = bTriggerOnce;
	return Task;
}

void UAbilityTask_WaitReplicatedEvent::Activate()
{
	if (!AbilitySystemComponent.IsValid())
	{
		EndTask();
		return;
	}

	FSimpleMulticastDelegate& EventDelegate = AbilitySystemComponent->AbilityReplicatedEventDelegate(EventType, GetAbilitySpecHandle(), GetActivationPredictionKey());
	EventDelegateHandle = EventDelegate.AddUObject(this, &ThisClass::OnEventReceived);

	// The event may have arrived before the task started.
	if (!AbilitySystemComponent->CallReplicatedEventDelegateIfSet(EventType, GetAbilitySpecHandle(), GetActivationPredictionKey()) && IsForRemoteClient())
	{
		SetWaitingOnRemotePlayerData();
	}
}

void UAbilityTask_WaitReplicatedEvent::OnEventReceived()
{
	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->ConsumeGenericReplicatedEvent(EventType, GetAbilitySpecHandle(), GetActivationPredictionKey());
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnEvent.Broadcast();
	}

	if (bTriggerOnce)
	{
		EndTask();
	}
	else if (IsForRemoteClient())
	{
		ClearWaitingOnRemotePlayerData();
	}
}

void UAbilityTask_WaitReplicatedEvent::OnDestroy(bool bInOwnerFinished)
{
	if (AbilitySystemComponent.IsValid() && EventDelegateHandle.IsValid())
	{
		AbilitySystemComponent->AbilityReplicatedEventDelegate(EventType, GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(EventDelegateHandle);
	}

	Super::OnDestroy(bInOwnerFinished);
}
