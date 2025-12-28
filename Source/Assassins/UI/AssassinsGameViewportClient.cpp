// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/AssassinsGameViewportClient.h"
#include "Character/AssassinsCharacter.h"

void UAssassinsGameViewportClient::MouseMove(FViewport* InViewport, int32 X, int32 Y)
{
	Super::MouseMove(InViewport, X, Y);

	APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController();
	FHitResult Hit;
	bool bHitSuccessful = false;
	
	if (PC)
	{
		bHitSuccessful = PC->GetHitResultUnderCursor(ECollisionChannel::ECC_GameTraceChannel1 /*targeting trace*/, true, Hit);
	}

	AAssassinsCharacter* CursorTarget = nullptr;

	if (bHitSuccessful)
	{
		CursorTarget = Cast<AAssassinsCharacter>(Hit.GetActor());
	}

	if (CursorTarget)
	{
		OnCursorTargetSet.ExecuteIfBound(CursorTarget);
	}
	else
	{
		OnCursorTargetCleared.ExecuteIfBound();
	}

	if (Viewport)
	{
		FIntPoint ViewportSizeXY = Viewport->GetSizeXY();

		if (X <= 10)
		{
			OnCursorAtViewportEdge.ExecuteIfBound(ECardinalDirection::Left);
		}
		else if (X > ViewportSizeXY.X - 10)
		{
			OnCursorAtViewportEdge.ExecuteIfBound(ECardinalDirection::Right);
		}

		if (Y <= 10)
		{
			OnCursorAtViewportEdge.ExecuteIfBound(ECardinalDirection::Up);
		}
		else if (Y > ViewportSizeXY.Y - 10)
		{
			OnCursorAtViewportEdge.ExecuteIfBound(ECardinalDirection::Down);
		}
	}
}
