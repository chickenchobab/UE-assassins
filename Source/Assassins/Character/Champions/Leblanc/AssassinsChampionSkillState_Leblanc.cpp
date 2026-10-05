// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Champions/Leblanc/AssassinsChampionSkillState_Leblanc.h"

#include "Abilities/GameplayAbility.h"
#include "Character/AssassinsChampion.h"
#include "NativeGameplayTags.h"

namespace LeblancMimic
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ABILITY4, "Ability.Ability4");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_MIMIC_ABILITY1, "Event.Champion.Leblanc.Mimic.Ability1");
};

void UAssassinsChampionSkillState_Leblanc::Initialize()
{
	// The ultimate cast before any other ability mimics Ability1, as the default of B_Leblanc had it.
	MimicAbilityEventTag = LeblancMimic::TAG_EVENT_MIMIC_ABILITY1;
}

void UAssassinsChampionSkillState_Leblanc::SetMimicAbility(const UGameplayAbility& CastAbility, const FGameplayTag& EventTag)
{
	if (CastAbility.GetAssetTags().HasTagExact(LeblancMimic::TAG_ABILITY_ABILITY4))
	{
		return;
	}

	MimicAbilityEventTag = EventTag;
}

void UAssassinsChampionSkillState_Leblanc::SetMimicAbilityOf(const UGameplayAbility& CastAbility, const FGameplayTag& EventTag)
{
	if (UAssassinsChampionSkillState_Leblanc* LeblancState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Leblanc>(CastAbility.GetAvatarActorFromActorInfo()))
	{
		LeblancState->SetMimicAbility(CastAbility, EventTag);
	}
}
