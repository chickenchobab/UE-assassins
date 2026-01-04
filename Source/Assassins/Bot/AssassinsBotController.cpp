// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/AssassinsBotController.h"
#include "Character/AssassinsCharacterWithAbilities.h"

AAssassinsBotController::AAssassinsBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanSetTeamId = false;
}

void AAssassinsBotController::SetGenericTeamId(const FGenericTeamId& NewID)
{
	bCanSetTeamId = true;

	if (AAssassinsCharacterWithAbilities* Bot = GetPawn<AAssassinsCharacterWithAbilities>())
	{
		Bot->SetGenericTeamId(NewID);
	}

	bCanSetTeamId = false;
}
