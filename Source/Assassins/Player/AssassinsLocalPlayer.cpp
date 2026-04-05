// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AssassinsLocalPlayer.h"
#include "Player/AssassinsPlayerController.h"
#include "Character/AssassinsCharacter.h"

void UAssassinsLocalPlayer::CallOrRegister_OnLocalPlayerRestarted(FLocalCharacterRestartedDelegate::FDelegate&& Delegate)
{
	auto CallAndRegisterDelegate = [Delegate = MoveTemp(Delegate)](APlayerController* PC) mutable {
		if (AAssassinsPlayerController* AssassinsPC = Cast<AAssassinsPlayerController>(PC))
		{
			if (AssassinsPC->GetPlayerRestarted())
			{
				Delegate.ExecuteIfBound(AssassinsPC->GetCharacter());
			}
			else
			{
				AssassinsPC->OnPlayerRestarted.Add(Delegate);
			}
		}
	};

	if (APlayerController* PC = GetPlayerController(GetWorld()))
	{
		CallAndRegisterDelegate(PC);
	}
	else
	{
		OnPlayerControllerChanged().AddLambda(CallAndRegisterDelegate);
	}
}