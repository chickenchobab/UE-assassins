// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AssassinsLocalPlayer.h"
#include "Player/AssassinsPlayerController.h"
#include "Character/AssassinsCharacter.h"

void UAssassinsLocalPlayer::CallAndRegister_OnLocalPlayerRestarted(FLocalCharacterRestartedDelegate::FDelegate&& Delegate)
{
	if (AAssassinsPlayerController* PC = Cast<AAssassinsPlayerController>(GetPlayerController(GetWorld())))
	{
		if (PC->GetPlayerRestarted())
		{
			Delegate.Execute(PC->GetCharacter());
		}
		else
		{
			PC->OnPlayerRestarted.Add(MoveTemp(Delegate));
		}
	}
}