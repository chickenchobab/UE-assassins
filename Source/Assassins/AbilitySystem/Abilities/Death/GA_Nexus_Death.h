// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Death/GA_Death.h"
#include "GA_Nexus_Death.generated.h"

/**
 * UGA_Nexus_Death
 *
 * The nexus falling ends the game: once it has collapsed, everyone goes back to the front end.
 * Was the GA_Nexus_Death blueprint.
 */
UCLASS(Abstract)
class UGA_Nexus_Death : public UGA_Death
{
	GENERATED_BODY()

protected:

	//~UGA_Death interface
	virtual void OnMontageEnd() override;
	//~End of UGA_Death interface
};
