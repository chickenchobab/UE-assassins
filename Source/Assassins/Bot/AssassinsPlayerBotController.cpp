// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/AssassinsPlayerBotController.h"
#include "Player/AssassinsPlayerState.h"
#include "GameModes/AssassinsGameMode.h"
#include "AssassinsLogCategories.h"

AAssassinsPlayerBotController::AAssassinsPlayerBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AAssassinsPlayerBotController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogAssassinsTeams, Error, TEXT("You can't set the team ID on a player bot controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AAssassinsPlayerBotController::GetGenericTeamId() const
{
	if (IAssassinsTeamAgentInterface* PSWithTeamInterface = Cast<IAssassinsTeamAgentInterface>(GetPlayerState<APlayerState>()))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

void AAssassinsPlayerBotController::ServerRestartController()
{
	if (GetNetMode() == NM_Client)
	{
		return;
	}

	ensure((GetPawn() == nullptr) && IsInState(NAME_Inactive));

	if (IsInState(NAME_Inactive) || (IsInState(NAME_Spectating)))
	{
		AAssassinsGameMode* const GameMode = GetWorld()->GetAuthGameMode<AAssassinsGameMode>();

		if ((GameMode == nullptr) || !GameMode->ControllerCanRestart(this))
		{
			return;
		}

		// If we're still attached to a Pawn, leave it
		if (GetPawn() != nullptr)
		{
			UnPossess();
		}

		// Re-enable input, similar to code in ClientRestart
		ResetIgnoreInputFlags();

		GameMode->RestartPlayer(this);
	}
}
