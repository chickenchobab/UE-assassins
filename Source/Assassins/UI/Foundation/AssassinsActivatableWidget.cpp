// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Foundation/AssassinsActivatableWidget.h"
#include "GameModes/AssassinsGameState.h"


UAssassinsActivatableWidget::UAssassinsActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TOptional<FUIInputConfig> UAssassinsActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputConfig)
	{
	case EAssassinsWidgetInputMode::GameAndMenu:
		return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode, false);
	case EAssassinsWidgetInputMode::Game:
		return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode, false);
	case EAssassinsWidgetInputMode::Menu:
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, false);
	case EAssassinsWidgetInputMode::Default:
	default:
		return TOptional<FUIInputConfig>();
	}
}

AAssassinsGameState* UAssassinsActivatableWidget::GetAssassinsGameState() const
{
	return GetWorld()->GetGameStateChecked<AAssassinsGameState>();
}
