// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModularAIController.h"
#include "Teams/AssassinsTeamAgentInterface.h"

#include "AssassinsBotController.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class AAssassinsBotController : public AModularAIController, public IAssassinsTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	AAssassinsBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~IAssassinsTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewID) override;
	//~End of IAssassinsTeamAgentInterface interface

	bool CanSetPawnTeamId() const { return bCanSetTeamId; }

private:
	bool bCanSetTeamId;
};
