// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/AssassinsChampionSkillState.h"
#include "GameplayTagContainer.h"
#include "AssassinsChampionSkillState_Leblanc.generated.h"

class UGameplayAbility;

/**
 * UAssassinsChampionSkillState_Leblanc
 *
 * Which ability Leblanc's ultimate(Mimic) casts again: the last of Ability1 to Ability3 she cast. Was the
 * MimicAbilityEventTag of B_Leblanc.
 */
UCLASS()
class ASSASSINS_API UAssassinsChampionSkillState_Leblanc : public UAssassinsChampionSkillState
{
	GENERATED_BODY()

public:

	//~UAssassinsChampionSkillState interface
	virtual void Initialize() override;
	//~End of UAssassinsChampionSkillState interface

	// Called by Ability1 to Ability3 as they are cast, with the event their mimic answers. The mimics themselves(they
	// carry Ability.Ability4) do not count.
	void SetMimicAbility(const UGameplayAbility& CastAbility, const FGameplayTag& EventTag);

	// SetMimicAbility on the skill state of the ability's avatar, when it is Leblanc's.
	static void SetMimicAbilityOf(const UGameplayAbility& CastAbility, const FGameplayTag& EventTag);

	const FGameplayTag& GetMimicAbilityEventTag() const { return MimicAbilityEventTag; }

private:

	UPROPERTY()
	FGameplayTag MimicAbilityEventTag;
};
