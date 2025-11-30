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

	if (AAssassinsCharacter* HitCharacter = Cast<AAssassinsCharacter>(Hit.GetActor()))
	{
		UE_LOG(LogTemp, Display, TEXT("Mouse hit a character [%s]"), *GetNameSafe(HitCharacter));
	}
}
