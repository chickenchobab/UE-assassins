// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"
#include "LoadingProcessInterface.h"
#include "ControlFlowNode.h"
#include "CommonUserTypes.h"

#include "AssassinsFrontendStateComponent.generated.h"

class UAssassinsExperienceDefinition;
class UCommonActivatableWidget;
class FControlFlow;

/**
 * Me: A component added when the player wants to see frontend menu.
 */
UCLASS(Abstract)
class ASSASSINS_API UAssassinsFrontendStateComponent : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()
	
public:

	UAssassinsFrontendStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

	//~ILoadingProcessInterface interface
	virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//~End of ILoadingProcessInterface interface

	UFUNCTION(BlueprintImplementableEvent, Category = ChampionSelection)
	void UpdateChampionSelectionScreen(const FPrimaryAssetId& ChampionRef, bool bIsChampionEnabled);

private:

	void OnExperienceLoaded(const UAssassinsExperienceDefinition* Experience);

	void FlowStep_WaitForUserInitialization(FControlFlowNodeRef SubFlow);
	void FlowStep_TryInitializeUser(FControlFlowNodeRef SubFlow);
	void FlowStep_TryShowMainScreen(FControlFlowNodeRef SubFlow);
	
	UFUNCTION()
	void OnUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);

	void TryShowChampionSelectionScreen();

private:

	bool bShouldShowLoadingScreen = true;

	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> MainScreenClass;
	UPROPERTY(EditAnywhere, Category = UI)
	TSoftClassPtr<UCommonActivatableWidget> ChampionSelectionScreenClass;

	UPROPERTY(BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<UCommonActivatableWidget> ChampionSelectionScreenWidget;

	TSharedPtr<FControlFlow> FrontendFlow;

	// If set, this is the in-progress press start screen task
	// Me: The logic would have run if the Press Start screen had been deactivated
	FControlFlowNodePtr InProgressPressStartScreen;
};
