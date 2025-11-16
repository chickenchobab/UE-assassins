// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFeatures/GameFeatureAction_WorldBase.h"

void UGameFeatureAction_WorldActionBase::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	// Me: Add to new world in the game instance
	GameInstanceStartHandles.FindOrAdd(Context) = FWorldDelegates::OnStartGameInstance.AddUObject(
		this, &UGameFeatureAction_WorldActionBase::HandleGameInstanceStart, FGameFeatureStateChangeContext(Context)
	);

	// Add to any worlds with associated game instances that have already been initialized
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			AddToWorld(WorldContext, Context);
		}
	}
}

void UGameFeatureAction_WorldActionBase::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FDelegateHandle* FoundHandle = GameInstanceStartHandles.Find(Context);
	if (ensure(FoundHandle))
	{
		FWorldDelegates::OnStartGameInstance.Remove(*FoundHandle);
	}
}

void UGameFeatureAction_WorldActionBase::HandleGameInstanceStart(UGameInstance* GameInstance, FGameFeatureStateChangeContext ChangeContext)
{
	if (FWorldContext* WorldContext = GameInstance->GetWorldContext())
	{
		if (ChangeContext.ShouldApplyToWorldContext(*WorldContext))
		{
			AddToWorld(*WorldContext, ChangeContext);
		}
	}
}
