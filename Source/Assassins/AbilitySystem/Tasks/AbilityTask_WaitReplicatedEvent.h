#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AbilityTask_WaitReplicatedEvent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitReplicatedEventDelegate);

UCLASS()
class ASSASSINS_API UAbilityTask_WaitReplicatedEvent : public UAbilityTask
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FWaitReplicatedEventDelegate OnEvent;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_WaitReplicatedEvent* WaitReplicatedEvent(UGameplayAbility* OwningAbility, EAbilityCustomReplicatedEvent CustomEvent, bool bTriggerOnce = true);

	virtual void Activate() override;

protected:

	virtual void OnDestroy(bool bInOwnerFinished) override;

private:

	void OnEventReceived();

	EAbilityGenericReplicatedEvent::Type EventType = EAbilityGenericReplicatedEvent::MAX;

	bool bTriggerOnce = true;

	FDelegateHandle EventDelegateHandle;
};
