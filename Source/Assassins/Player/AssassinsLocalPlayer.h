// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CommonLocalPlayer.h"
#include "AssassinsLocalPlayer.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FLocalCharacterRestartedDelegate, ACharacter*);

/**
 * 
 */
UCLASS()
class ASSASSINS_API UAssassinsLocalPlayer : public UCommonLocalPlayer
{
	GENERATED_BODY()
	
public:

	UAssassinsLocalPlayer();
};
