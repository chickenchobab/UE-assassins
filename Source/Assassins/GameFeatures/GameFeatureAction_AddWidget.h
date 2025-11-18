// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFeatures/GameFeatureAction_WorldBase.h"
#include "GameplayTagContainer.h"
#include "UIExtensionSystem.h"

#include "GameFeatureAction_AddWidget.generated.h"

class UCommonActivatableWidget;
struct FComponentRequestHandle;

USTRUCT()
struct FAssassinsHUDLayoutRequest
{
	GENERATED_BODY()

	// The layout widget to spawn
	UPROPERTY(EditAnywhere, Category=UI, meta=(AssetBundles="Client"))
	TSoftClassPtr<UCommonActivatableWidget> LayoutClass;

	// The layer to insert the widget in
	UPROPERTY(EditAnywhere, Category=UI, meta=(Categories="UI.Layer"))
	FGameplayTag LayerID;
};

USTRUCT()
struct FAssassinsHUDElementEntry
{
	GENERATED_BODY()

	// The widget to spawn
	UPROPERTY(EditAnywhere, Category=UI, meta=(AssetBundles="Client"))
	TSoftClassPtr<UUserWidget> WidgetClass;

	// The slot ID where we should place this widget
	UPROPERTY(EditAnywhere, Category=UI)
	FGameplayTag SlotID;
};


/**
 * GameFeatureAction responsible for adding widgets.
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Add Widgets"))
class UGameFeatureAction_AddWidget final: public UGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
	
public:
	//~UGameFeatureAction interface
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~End of UGameFeatureAction interface

private:
	// Layout to add to the HUD
	UPROPERTY(EditAnywhere, Category=UI, meta=(TitleProperty="{LayerID} -> {LayoutClass}"))
	TArray<FAssassinsHUDLayoutRequest> Layout;

	// Widgets to add to the HUD
	UPROPERTY(EditAnywhere, Category=UI, meta=(TitleProperty="{SlotID} -> {WidgetClass}"))
	TArray<FAssassinsHUDElementEntry> Widgets;

private:

	// Me: Save layouts and widgets of this action
	struct FPerActorData
	{
		TArray<TWeakObjectPtr<UCommonActivatableWidget>> LayoutsAdded;
		TArray<FUIExtensionHandle> ExtensionHandles;
	};

	struct FPerContextData
	{
		TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;
		TMap<FObjectKey, FPerActorData> ActorData;
	};

	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;

	//~UGameFeatureAction_WorldActionBase interface 
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) override;
	//~End of UGameFeatureAction_WorldActionBase interface

	void Reset(FPerContextData& ActiveData);

	void HandleActorExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext);

	void AddWidgets(AActor* Actor, FPerContextData& ActiveData);
	void RemoveWidgets(AActor* Actor, FPerContextData& ActiveData);
};
