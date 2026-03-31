// Fill out your copyright notice in the Description page of Project Settings.


#include "System/AssassinsGameInstance.h"
#include "Components/GameFrameworkComponentManager.h"
#include "AssassinsGameplayTags.h"
#include "CommonSessionSubsystem.h"
#include "OnlineSubsystemUtils.h"

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

	if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>(this))
	{
		SessionSubsystem->bUseBeacons = false;
	}
}

void UAssassinsGameInstance::StartSession()
{
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
	check(SessionInterface.IsValid());

	FNamedOnlineSession* Session = SessionInterface->GetNamedSession(NAME_GameSession);
	check(Session);
	SessionInterface->StartSession(NAME_GameSession);
}