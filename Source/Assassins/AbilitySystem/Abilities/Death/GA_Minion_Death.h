// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Death/GA_Death.h"
#include "GA_Minion_Death.generated.h"

/**
 * UGA_Minion_Death
 *
 * A minion lets its attack come to an end before it falls. Was the GA_Minion_Death blueprint.
 */
UCLASS(Abstract)
class UGA_Minion_Death : public UGA_Death
{
	GENERATED_BODY()

protected:

	//~UGA_Death interface
	virtual void PlayDeath() override;
	//~End of UGA_Death interface

private:

	UFUNCTION()
	void OnAttackingRemoved();

	UFUNCTION()
	void OnActivatingAttackRemoved();
};
