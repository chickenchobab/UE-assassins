// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CommonGameViewportClient.h"
#include "AssassinsGameViewportClient.generated.h"

class AAssassinsCharacter;

enum class ECardinalDirection : uint8
{
	Up,
	Down,
	Left,
	Right
};

DECLARE_DELEGATE_OneParam(FAssassinsOnCursorTargetSet, AAssassinsCharacter*);
DECLARE_DELEGATE(FAssassinsOnCursorTargetCleared);
DECLARE_MULTICAST_DELEGATE_OneParam(FAssassinsOnCursorAtViewportEdge, ECardinalDirection);

UCLASS(BlueprintType)
class ASSASSINS_API UAssassinsGameViewportClient : public UCommonGameViewportClient
{
	GENERATED_BODY()
	
public:

	FAssassinsOnCursorTargetSet OnCursorTargetSet;
	FAssassinsOnCursorTargetCleared OnCursorTargetCleared;

	FAssassinsOnCursorAtViewportEdge OnCursorAtViewportEdge;

protected:
	
	//~FViewportClient interface
	virtual void MouseMove(FViewport* Viewport, int32 X, int32 Y) override;
	//~End of FViewportClient interface
};
