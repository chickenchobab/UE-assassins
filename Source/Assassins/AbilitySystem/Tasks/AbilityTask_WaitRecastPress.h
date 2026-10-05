// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "AbilityTask_WaitRecastPress.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitRecastPressDelegate);

/**
 * UAbilityTask_WaitRecastPress
 *
 * Waits for the input of the ability to be pressed again, for a recast both sides wait for, as
 * UAbilityTask_WaitInputPress does: the press of the owning client reaches the server with a prediction key of its own.
 * Where the input is read(the owning client, or the server for its own players), a press while the avatar has one of
 * the blocking tags is not taken, e.g. one during the cast of another ability(Status.Channeling), which the recast
 * would cut short. The task stops listening while a blocking tag is on, so that the server never hears of such a press
 * either. The server of a remote client listens all along, for the presses its client sends.
 */
UCLASS()
class ASSASSINS_API UAbilityTask_WaitRecastPress : public UAbilityTask
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FWaitRecastPressDelegate OnPress;

	// With bTestAlreadyPressed, an input already down as the task starts counts as a press, unless a blocking tag is on
	// then. Once a blocking tag is gone, only a new press counts.
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_WaitRecastPress* WaitRecastPress(UGameplayAbility* OwningAbility, FGameplayTagContainer BlockingTags, bool bTestAlreadyPressed = false);

	//~UGameplayTask interface
	virtual void Activate() override;
	//~End of UGameplayTask interface

protected:

	//~UGameplayTask interface
	virtual void OnDestroy(bool bInOwnerFinished) override;
	//~End of UGameplayTask interface

private:

	void OnPressCallback();
	void OnBlockingTagChanged(const FGameplayTag Tag, int32 NewCount);

	bool IsBlocked() const;
	void StartListening();
	void StopListening();

	FGameplayTagContainer BlockingTags;
	bool bTestInitialState = false;

	FDelegateHandle PressDelegateHandle;
	TArray<TPair<FGameplayTag, FDelegateHandle>> BlockingTagDelegateHandles;
};
