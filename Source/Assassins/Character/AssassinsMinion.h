// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/AssassinsCharacterWithAbilities.h"
#include "AssassinsMinion.generated.h"

class UAssassinsHealthComponent;
class UAssassinsMontageWithTiming;

/**
 * AAssassinsMinion
 *
 * Native parent of B_MinionBase. Holds the minion settings the attack abilities read.
 * The names match the blueprint variables they replaced.
 * Gives the aggro tag of attacking a minion to whoever damages it, which the minions use to pick their target.
 */
UCLASS(Abstract)
class AAssassinsMinion : public AAssassinsCharacterWithAbilities
{
	GENERATED_BODY()

public:

	AAssassinsMinion();

protected:

	//~AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

private:

	// Gives the aggro tag to whoever damaged the minion. A heal draws no aggro. Was the GrantAggroTag event of
	// B_MinionBase, which gave it for a heal too.
	UFUNCTION()
	void GrantAggroTagToInstigator(UAssassinsHealthComponent* ChangedHealthComponent, float OldValue, float NewValue, AActor* HealthInstigator);

public:

	// Double, to match the blueprint variable it replaces.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Minion")
	double AttackRange = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Minion")
	bool IsSiegeMinion = false;

	// The attack montages this kind of minion picks from. Each minion blueprint fills it with its own.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Minion")
	TArray<TObjectPtr<UAssassinsMontageWithTiming>> MontageDataArray;
};
