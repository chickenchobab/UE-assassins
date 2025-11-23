// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Messaging/CommonGameDialog.h"
#include "AssassinsConfirmationScreen.generated.h"

class UCommonTextBlock;
class UCommonRichTextBlock;
class UDynamicEntryBox;
class UCommonBorder;


/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class UAssassinsConfirmationScreen : public UCommonGameDialog
{
	GENERATED_BODY()
	
public:
	//~UCommonGameDialog interface
	virtual void SetupDialog(UCommonGameDialogDescriptor* Descriptor, FCommonMessagingResultDelegate ResultCallback) override;
	virtual void KillDialog() override;
	//~End of UCommonGameDialog interface

protected:
	//UUserWidget interface
	virtual void NativeOnInitialized() override;
	//~End of UUserWidget interface

	virtual void CloseConfirmationWindow(ECommonMessagingResult Result);

private:
	
	UFUNCTION()
	FEventReply HandleTapToCloseZoneMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	FCommonMessagingResultDelegate OnResultCallback;

private:
	UPROPERTY(Meta = (BindWIdget))
	TObjectPtr<UCommonTextBlock> Text_Title;

	UPROPERTY(Meta = (BindWIdget))
	TObjectPtr<UCommonRichTextBlock> RichText_Description;

	UPROPERTY(Meta = (BindWIdget))
	TObjectPtr<UDynamicEntryBox> EntryBox_Buttons;

	UPROPERTY(Meta = (BindWidget))
	TObjectPtr<UCommonBorder> Border_TapToCloseZone;

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle CancelAction;
};
