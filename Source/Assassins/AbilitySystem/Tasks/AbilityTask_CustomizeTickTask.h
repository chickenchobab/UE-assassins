// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_CustomizeTickTask.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTickTaskDelegate, float, DeltaTime);

/**
 * Bind custom logic to TickTask, by a delegate
 */
UCLASS()
class ASSASSINS_API UAbilityTask_CustomizeTickTask : public UAbilityTask
{
	GENERATED_BODY()
	
public:
    UAbilityTask_CustomizeTickTask();

    UFUNCTION(BlueprintCallable, Category = "AbilityTasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
    static UAbilityTask_CustomizeTickTask* CustomizeTickTask(UGameplayAbility* OwningAbility);

    //~UGameplayTask interface
    virtual void TickTask(float DeltaTime) override;
    //~End of UGameplayTask interface

    UPROPERTY(BlueprintAssignable)
    FOnTickTaskDelegate OnTickTask;
};
