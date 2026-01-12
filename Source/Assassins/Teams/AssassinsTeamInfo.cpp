// Fill out your copyright notice in the Description page of Project Settings.


#include "Teams/AssassinsTeamInfo.h"
#include "Teams/AssassinsTeamSubsystem.h"

AAssassinsTeamInfo::AAssassinsTeamInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AAssassinsTeamInfo::BeginPlay()
{
	Super::BeginPlay();

	TryRegisterWithTeamSubsystem();
}

void AAssassinsTeamInfo::TryRegisterWithTeamSubsystem()
{
	if (TeamId != INDEX_NONE)
	{
		UAssassinsTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UAssassinsTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			RegisterWithTeamSubsystem(TeamSubsystem);
		}
	}
}

void AAssassinsTeamInfo::RegisterWithTeamSubsystem(UAssassinsTeamSubsystem* TeamSubsystem)
{
	TeamSubsystem->RegisterTeamInfo(this);
}
