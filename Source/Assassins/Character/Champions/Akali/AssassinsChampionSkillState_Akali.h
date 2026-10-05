// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/AssassinsChampionSkillState.h"
#include "AssassinsChampionSkillState_Akali.generated.h"

class AAssassinsProjectile;
class UCapsuleComponent;

/**
 * UAssassinsChampionSkillState_Akali
 *
 * What Akali's abilities share: the shuriken of Ability3, which the shroud of Ability2 watches for, and the capsule the
 * ultimate(Ability4 and its recast) finds its targets with. Was held by AAssassinsChampion_Akali. The capsule was a
 * component of B_Akali found by its name: the skill state makes it now.
 */
UCLASS()
class ASSASSINS_API UAssassinsChampionSkillState_Akali : public UAssassinsChampionSkillState
{
	GENERATED_BODY()

public:

	//~UAssassinsChampionSkillState interface
	virtual void Initialize() override;
	//~End of UAssassinsChampionSkillState interface

	// Each ability holds the capsule while it dashes: the collision stays on until the last one lets go, and each
	// ability binds and unbinds only its own handler.
	UCapsuleComponent* AcquireUltimateCapsule(const UObject* User);
	void ReleaseUltimateCapsule(const UObject* User);

	// For a look at the capsule without holding it.
	const UCapsuleComponent* GetUltimateCapsule() const { return UltimateCapsule; }

	// The shuriken thrown by Ability3. Set on the server only, which alone throws it.
	UPROPERTY()
	TObjectPtr<AAssassinsProjectile> SpawnedProjectile;

private:

	// Made in Initialize, on Akali's root.
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> UltimateCapsule;

	TSet<TWeakObjectPtr<const UObject>> UltimateCapsuleUsers;
};
