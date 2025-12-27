// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"
#include "LoadingProcessInterface.h"

#include "AssassinsExperienceStateComponent.generated.h"

namespace UE::GameFeatures { struct FResult; }
class UAssassinsExperienceDefinition;
class UAssassinsCameraMode;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAssassinsExperienceLoaded, const UAssassinsExperienceDefinition* /*Experience*/);

enum class EAssassinsExperienceLoadState
{
	Unloaded,
	Loading,
	LoadingGameFeatures,
	// LoadingChaosTestingDelay,
	ExecutingActions,
	Loaded,
	Deactivating
};

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsExperienceStateComponent : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()
	
public:
	//~UActorComponent interface
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	//~ILoadingProcessInterface interface
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//~End of ILoadingProcessInterface interface

	// Tries to set the current experience, either a UI or gameplay one
	void SetCurrentExperience(FPrimaryAssetId ExperienceId);

	// Ensures the delegate is called once the experience has been loaded,
	// before others are called.
	// However, if the experience has already loaded, calls the delegate immediately.
	void CallOrRegister_OnExperienceLoaded_HighPriority(FOnAssassinsExperienceLoaded::FDelegate&& Delegate);

	// Ensures the delegate is called once the experience has been loaded
	// If the experience has already loaded, calls the delegate immediately
	void CallOrRegister_OnExperienceLoaded(FOnAssassinsExperienceLoaded::FDelegate&& Delegate);

	// Ensures the delegate is called once the experience has been loaded
	// If the experience has already loaded, calls the delegate immediately
	void CallOrRegister_OnExperienceLoaded_LowPriority(FOnAssassinsExperienceLoaded::FDelegate&& Delegate);

	// This returns the current experience if it is fully loaded, asserting otherwise
	// (i.e., if tou called it too soon)
	const UAssassinsExperienceDefinition* GetCurrentExperienceChecked() const;

	bool IsExperienceLoaded() const;

	const TSubclassOf<UAssassinsCameraMode> GetCurrentExperienceCameraMode() const;

private:

	void StartExperienceLoad();
	void OnExperienceLoadComplete();
	void OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result);
	void OnExperienceFullLoadCompleted();

	void OnActionDeactivationCompleted();
	void OnAllActionsDeactivated();

private:

	UPROPERTY()
	TObjectPtr<const UAssassinsExperienceDefinition> CurrentExperience;

	EAssassinsExperienceLoadState LoadState = EAssassinsExperienceLoadState::Unloaded;

	int32 NumGameFeaturePluginsLoading = 0;
	TArray<FString> GameFeaturePluginURLs;

	int32 NumObservedPausers = 0;
	int32 NumExpectedPausers = 0;

	/**
	 * Delegate called when the experience has finished loading just before others
	 * (e.g., subsystems that set up for regular gameplay)
	 */
	FOnAssassinsExperienceLoaded OnExperienceLoaded_HighPriority;

	/** Delegate called when the experience has finished loading */
	FOnAssassinsExperienceLoaded OnExperienceLoaded;

	/** Delegate called when the experience has finished loading */
	FOnAssassinsExperienceLoaded OnExperienceLoaded_LowPriority;
};
