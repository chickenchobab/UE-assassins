// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Bot/AssassinsBotController.h"

#include "AssassinsPlayerBotController.generated.h"

/**
 *  The controller class used by player(champion) bots in this project
 */
UCLASS(Blueprintable)
class ASSASSINS_API AAssassinsPlayerBotController : public AAssassinsBotController
{
	GENERATED_BODY()
	
public:
	AAssassinsPlayerBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~IAssassinsTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~End of IAssassinsTeamAgentInterface interface

	// Attemps to restart the controller (e.g., to respawn it)
	void ServerRestartController();
};
