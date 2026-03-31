// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CommonActivatableWidget.h"
#include "AssassinsActivatableWidget.generated.h"

struct FUIInputConfig;
class AAssassinsGameState;

UENUM(BlueprintType)
enum class EAssassinsWidgetInputMode : uint8
{
	Default,
	GameAndMenu,
	Game,
	Menu
};

/**
 * An activatable widget that automatically drives the desired input config when activated
 */
UCLASS(Abstract, Blueprintable)
class ASSASSINS_API UAssassinsActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UAssassinsActivatableWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	//~UCommonActivatableWidget interface
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const;
	//~End of UCommonActivatableWidget interface

	UFUNCTION(BlueprintPure, Category = GameState)
	AAssassinsGameState* GetAssassinsGameState() const;

protected:
	/** The desired input mode to use while this UI is activated, for example do you want key presses to still reach the game/player controller? */
	UPROPERTY(EditDefaultsOnly, Category = Input)
	EAssassinsWidgetInputMode InputConfig = EAssassinsWidgetInputMode::Default;

	/** The desired mouse behavior when the game gets input. */
	UPROPERTY(EditDefaultsOnly, Category = Input)
	EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};
