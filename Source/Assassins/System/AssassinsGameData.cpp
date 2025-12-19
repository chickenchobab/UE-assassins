// Fill out your copyright notice in the Description page of Project Settings.


#include "System/AssassinsGameData.h"
#include "System/AssassinsAssetManager.h"

UAssassinsGameData::UAssassinsGameData()
{
}

const UAssassinsGameData& UAssassinsGameData::Get()
{
	return UAssassinsAssetManager::Get().GetGameData();
}