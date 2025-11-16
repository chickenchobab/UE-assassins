// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"

#include "GameFeatureAction_WorldBase.generated.h"

/**
 * Base class for GameFeatureAction that wish to do something world specific
 */
UCLASS(Abstract)
class ASSASSINS_API UGameFeatureAction_WorldActionBase : public UGameFeatureAction
{
	GENERATED_BODY()
	
public:
	//~UGameFeatureAction interface
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~End of UGameFeatureAction interface

private:
	void HandleGameInstanceStart(UGameInstance* GameInstance, FGameFeatureStateChangeContext ChangeContext);

	/** Override with the action-specific logic */
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) PURE_VIRTUAL(UGameFeatureAction_WorldActionBase::AddToWorld, /*function body*/);

private:
	// Me: Delegates which are broadcasted from UGameInstance::StartGameInstance
	TMap<FGameFeatureStateChangeContext, FDelegateHandle> GameInstanceStartHandles;
};
