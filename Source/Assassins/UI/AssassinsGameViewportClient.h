// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CommonGameViewportClient.h"
#include "AssassinsGameViewportClient.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class ASSASSINS_API UAssassinsGameViewportClient : public UCommonGameViewportClient
{
	GENERATED_BODY()
	
protected:
	
	//~FViewportClient interface
	virtual void MouseMove(FViewport* Viewport, int32 X, int32 Y) override;
	//~End of FViewportClient interface
};
