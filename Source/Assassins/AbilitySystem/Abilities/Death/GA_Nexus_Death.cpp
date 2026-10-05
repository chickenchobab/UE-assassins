// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Death/GA_Nexus_Death.h"

#include "Teams/AssassinsTeamBaseActor.h"

void UGA_Nexus_Death::OnMontageEnd()
{
	if (AAssassinsTeamBaseActor* Nexus = Cast<AAssassinsTeamBaseActor>(GetAvatarActorFromActorInfo()))
	{
		Nexus->ReturnAllPlayersToFrontend();
	}

	Super::OnMontageEnd();
}
