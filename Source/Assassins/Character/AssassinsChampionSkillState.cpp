// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/AssassinsChampionSkillState.h"
#include "Character/AssassinsChampion.h"

AAssassinsChampion* UAssassinsChampionSkillState::GetChampion() const
{
	return GetTypedOuter<AAssassinsChampion>();
}
