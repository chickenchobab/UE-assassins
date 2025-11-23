// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/AssassinsGameState.h"
#include "AssassinsExperienceStateComponent.h"
#include "Player/AssassinsPlayerState.h"

AAssassinsGameState::AAssassinsGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	ExperienceComponent = CreateDefaultSubobject<UAssassinsExperienceStateComponent>(TEXT("ExperienceComponent"));
}
}
