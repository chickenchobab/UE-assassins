// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/AssassinsPlayerController.h"
#include "Player/AssassinsPlayerState.h"
#include "GameFramework/Pawn.h"
#include "Character/AssassinsCharacter.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AAssassinsPlayerController::AAssassinsPlayerController()
{
	
}

UAssassinsAbilitySystemComponent* AAssassinsPlayerController::GetAssassinsAbilitySystemComponent() const
{
	const AAssassinsPlayerState* AssassinsPS = GetPlayerState<AAssassinsPlayerState>();
	return AssassinsPS ? AssassinsPS->GetAssassinsAbilitySystemComponent() : nullptr;
}

void AAssassinsPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponent())
	{
		AssassinsASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

