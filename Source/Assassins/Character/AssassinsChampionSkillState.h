// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/Object.h"
#include "AssassinsChampionSkillState.generated.h"

class AAssassinsChampion;

/**
 * UAssassinsChampionSkillState
 *
 * What a champion's abilities share with each other and with the actors they spawn, e.g. Zed's shadows.
 * The ability set of the champion names the class(UAssassinsAbilitySet::SkillStateClass), and the champion makes one on
 * every machine once its pawn data is known. It is not replicated: what it holds may differ between the machines, e.g.
 * the owning client keeps the shadows it predicts, the server the ones it replicates.
 */
UCLASS(Abstract)
class ASSASSINS_API UAssassinsChampionSkillState : public UObject
{
	GENERATED_BODY()

public:

	// Called once, right after the champion made it.
	virtual void Initialize() {}

	AAssassinsChampion* GetChampion() const;
};
