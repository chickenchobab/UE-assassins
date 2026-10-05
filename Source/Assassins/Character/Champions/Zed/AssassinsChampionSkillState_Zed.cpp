// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Champions/Zed/AssassinsChampionSkillState_Zed.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"

namespace ZedShadows
{
	// One from Ability2 and one from Ability4.
	static constexpr int32 MaxShadows = 2;
};

void UAssassinsChampionSkillState_Zed::AddShadow(AAssassinsZedShadow* Shadow)
{
	// A shadow destroyed without removing itself does not keep its place.
	Shadows.RemoveAll([](const TObjectPtr<AAssassinsZedShadow>& Existing) { return !IsValid(Existing); });

	if (IsValid(Shadow) && (Shadows.Num() < ZedShadows::MaxShadows))
	{
		Shadows.AddUnique(Shadow);
	}
}

void UAssassinsChampionSkillState_Zed::RemoveShadow(AAssassinsZedShadow* Shadow)
{
	Shadows.Remove(Shadow);
}
