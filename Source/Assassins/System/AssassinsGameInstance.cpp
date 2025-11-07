// Fill out your copyright notice in the Description page of Project Settings.


#include "System/AssassinsGameInstance.h"
#include "Components/GameFrameworkComponentManager.h"
#include "AssassinsGameplayTags.h"

void UAssassinsGameInstance::Init()
{
	Super::Init();
	
	// Register our custom init states
	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);

	if (ensure(ComponentManager))
	{
		ComponentManager->RegisterInitState(AssassinsGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(AssassinsGameplayTags::InitState_DataAvailable, false, AssassinsGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(AssassinsGameplayTags::InitState_DataInitialized, false, AssassinsGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(AssassinsGameplayTags::InitState_GameplayReady, false, AssassinsGameplayTags::InitState_DataInitialized);
	}
}
