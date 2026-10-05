// Fill out your copyright notice in the Description page of Project Settings.

#include "System/AssassinsDeveloperSettings.h"

#include "Character/AssassinsPawnData.h"
#include "Misc/App.h"

UAssassinsDeveloperSettings::UAssassinsDeveloperSettings()
{
}

FName UAssassinsDeveloperSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}
