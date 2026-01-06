// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ModularAIController.h"
#include "Teams/AssassinsTeamAgentInterface.h"

#include "AssassinsBotController.generated.h"

class UAssassinsTargetChasingComponent;

/**
 * 
 */
UCLASS(Blueprintable)
class AAssassinsBotController : public AModularAIController, public IAssassinsTeamAgentInterface
{
	GENERATED_BODY()

public:
	AAssassinsBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	//~End of AActor interface

	//~IAssassinsTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewID) override;
	FGenericTeamId GetGenericTeamId() const override;
	//~End of IAssassinsTeamAgentInterface interface

	UFUNCTION(BlueprintPure, Category = "Assassins|Bot")
	FGenericTeamId GetTeamId() const { return MyTeamID; }

	bool CanSetPawnTeamId() const { return bCanSetTeamId; }

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UAssassinsTargetChasingComponent> TargetChasingComponent;

protected:
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	bool bCanSetTeamId;

	FGenericTeamId MyTeamID;

	UPROPERTY()
	TWeakObjectPtr<AActor> CachedTarget;

	float CachedAcceptRadius;
};
