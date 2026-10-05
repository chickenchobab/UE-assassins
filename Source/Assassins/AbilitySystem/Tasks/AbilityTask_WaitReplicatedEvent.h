// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilityTask_WaitReplicatedEvent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitReplicatedEventDelegate);

/**
 * UAbilityTask_WaitReplicatedEvent
 *
 * Waits for a custom replicated event of the ability: one the owning client sent(SendPredictedEventToServer,
 * ServerSetReplicatedEvent) or the server sent(ClientSetReplicatedEvent). An event that arrived before the task
 * started counts. Only this event is consumed, and the task ends with the ability, which a delegate added with
 * CallOrAddReplicatedDelegate does not.
 */
UCLASS()
class ASSASSINS_API UAbilityTask_WaitReplicatedEvent : public UAbilityTask
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FWaitReplicatedEventDelegate OnEvent;

	// With bTriggerOnce off, the task keeps answering every time the event comes again.
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_WaitReplicatedEvent* WaitReplicatedEvent(UGameplayAbility* OwningAbility, EAbilityCustomReplicatedEvent CustomEvent, bool bTriggerOnce = true);

	//~UGameplayTask interface
	virtual void Activate() override;
	//~End of UGameplayTask interface

protected:

	//~UGameplayTask interface
	virtual void OnDestroy(bool bInOwnerFinished) override;
	//~End of UGameplayTask interface

private:

	void OnEventReceived();

	EAbilityGenericReplicatedEvent::Type EventType = EAbilityGenericReplicatedEvent::MAX;

	bool bTriggerOnce = true;

	FDelegateHandle EventDelegateHandle;
};
