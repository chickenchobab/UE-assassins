// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/AssassinsChampionSkillState.h"
#include "AssassinsChampionSkillState_Zed.generated.h"

class AAssassinsZedShadow;

/**
 * UAssassinsChampionSkillState_Zed
 *
 * Zed's shadows, which his Ability1 and Ability3 make imitate them. The shadows of Ability2 and Ability4 add and remove
 * themselves, those this machine shows(see AAssassinsZedShadow::RegisterWithOwner). Was the Shadows array of B_Zed.
 */
UCLASS()
class ASSASSINS_API UAssassinsChampionSkillState_Zed : public UAssassinsChampionSkillState
{
	GENERATED_BODY()

public:

	void AddShadow(AAssassinsZedShadow* Shadow);
	void RemoveShadow(AAssassinsZedShadow* Shadow);

	const TArray<TObjectPtr<AAssassinsZedShadow>>& GetShadows() const { return Shadows; }

private:

	UPROPERTY()
	TArray<TObjectPtr<AAssassinsZedShadow>> Shadows;
};
