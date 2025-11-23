// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Messaging/CommonMessagingSubsystem.h"
#include "AssassinsMessagingSubsystem.generated.h"

class UCommonGameDialog;

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsMessagingSubsystem : public UCommonMessagingSubsystem
{
	GENERATED_BODY()

public:
	//~USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~End of USubsystem interface
	
	//~UCommonMessagingSubsystem interface
	virtual void ShowConfirmation(UCommonGameDialogDescriptor* DialogDescriptor, FCommonMessagingResultDelegate ResultCallback = FCommonMessagingResultDelegate()) override;
	virtual void ShowError(UCommonGameDialogDescriptor* DialogDescriptor, FCommonMessagingResultDelegate ResultCallback = FCommonMessagingResultDelegate()) override;
	//~End of UCommonMessagingSubsystem interface

private:
	UPROPERTY()
	TSubclassOf<UCommonGameDialog> ConfirmationDialogClassPtr;

	UPROPERTY()
	TSubclassOf<UCommonGameDialog> ErrorDialogClassPtr;

	UPROPERTY(config)
	TSoftClassPtr<UCommonGameDialog> ConfirmationDialogClass;

	UPROPERTY(config)
	TSoftClassPtr<UCommonGameDialog> ErrorDialogClass;
};
