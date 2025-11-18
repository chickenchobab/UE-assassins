// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UI/Foundation/AssassinsActivatableWidget.h"
#include "AssassinsHUDLayout.generated.h"

/**
 * Widget used to lay out the player's HUD (typically specified by an Add Widgets action in the experience)
 */
UCLASS(Abstract, BlueprintType, Blueprintable, Meta = (DisplayName = "Assassins HUD Layout", Category = "Assassins|HUD"))
class ASSASSINS_API UAssassinsHUDLayout : public UAssassinsActivatableWidget
{
	GENERATED_BODY()

public:

	virtual void NativeOnInitialized() override;

protected:

	void HandleEscapeAction();

	// The menu to be displayed when the user presses the "Pause" or "Escape" button
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;
};
