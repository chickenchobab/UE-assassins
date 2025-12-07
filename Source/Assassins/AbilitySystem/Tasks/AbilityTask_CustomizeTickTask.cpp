// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Tasks/AbilityTask_CustomizeTickTask.h"

UAbilityTask_CustomizeTickTask::UAbilityTask_CustomizeTickTask()
{
    bTickingTask = true;
}

UAbilityTask_CustomizeTickTask* UAbilityTask_CustomizeTickTask::CustomizeTickTask(UGameplayAbility* OwningAbility)
{
    UAbilityTask_CustomizeTickTask* Node = NewAbilityTask<UAbilityTask_CustomizeTickTask>(OwningAbility);
    return Node;
}

void UAbilityTask_CustomizeTickTask::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnTickTask.Broadcast(DeltaTime);
    }
    else
    {
        EndTask();
    }
}
