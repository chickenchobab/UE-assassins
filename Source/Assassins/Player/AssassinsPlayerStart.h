// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerStart.h"
#include "AssassinsPlayerStart.generated.h"

enum class EAssassinsPlayerStartOccupancy
{
	Empty,
	Partial,
	Full
};

/**
 * PlayerStart for player bots(champions)
 */
UCLASS(Config = Game)
class ASSASSINS_API AAssassinsPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:

	AAssassinsPlayerStart(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	int32 GetTeamId() const { return TeamId; }

	EAssassinsPlayerStartOccupancy GetOccupancy(AController* const ControllerPawnToFit) const;
	void SetOccupied() { Occupancy = EAssassinsPlayerStartOccupancy::Full; }

protected:

	UPROPERTY(EditAnywhere, Category = "Assassins|Player")
	int32 TeamId;

	UPROPERTY(EditAnywhere, Category = "Assassins|Player")
	bool bOnePlayerPerSpot;

private:

	EAssassinsPlayerStartOccupancy Occupancy;
};
