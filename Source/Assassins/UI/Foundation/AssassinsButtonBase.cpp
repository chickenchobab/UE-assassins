// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Foundation/AssassinsButtonBase.h"
#include "CommonActionWidget.h"

void UAssassinsButtonBase::SetButtonText(const FText& InText)
{
	bOverride_ButtonText = InText.IsEmpty();
	ButtonText = InText;
	RefreshButtonText();
}

void UAssassinsButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshButtonText();
}

void UAssassinsButtonBase::UpdateInputActionWidget()
{
	Super::UpdateInputActionWidget();

	RefreshButtonText();
}

void UAssassinsButtonBase::RefreshButtonText()
{
	if (bOverride_ButtonText || ButtonText.IsEmpty())
	{
		// Me: CommonButtonBase has input action widget.
		if (InputActionWidget)
		{
			const FText ActionDisplayText = InputActionWidget->GetDisplayText();
			if (!ActionDisplayText.IsEmpty())
			{
				UpdateButtonText(ActionDisplayText);
				return;
			}
		}
	}

	UpdateButtonText(ButtonText);
}
