// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityTestDriver.h"
#include "AbilityTestSession.h"
#include "AbilityTestSteps.h"

#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "Character/AssassinsChampion.h"
#include "Player/AssassinsPlayerController.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

FAbilityTestAim FAbilityTestDriver::AimAt(AActor* ClientTarget)
{
	FAbilityTestAim Aim;
	Aim.Target = ClientTarget;

	if (ClientTarget)
	{
		Aim.Location = ClientTarget->GetActorLocation();
		if (const ACharacter* Character = Cast<ACharacter>(ClientTarget))
		{
			Aim.Location.Z -= Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
	}
	return Aim;
}

void FAbilityTestDriver::SetAim(const FAbilityTestPlayer& Player, const FAbilityTestAim& Aim)
{
	if (AAssassinsPlayerController* Controller = Player.ClientController.Get())
	{
		Controller->SetAimOverride(Aim.Location, Aim.Target.Get());
	}
}

void FAbilityTestDriver::ClearAim(const FAbilityTestPlayer& Player)
{
	if (AAssassinsPlayerController* Controller = Player.ClientController.Get())
	{
		Controller->ClearAimOverride();
	}
}

void FAbilityTestDriver::PressInput(const FAbilityTestPlayer& Player, const FGameplayTag& InputTag)
{
	if (UAssassinsAbilitySystemComponent* AbilitySystem = Player.GetClientAbilitySystem())
	{
		FGameplayTag Tag = InputTag;
		AbilitySystem->AbilityInputTagPressed(Tag);
	}
}

void FAbilityTestDriver::ReleaseInput(const FAbilityTestPlayer& Player, const FGameplayTag& InputTag)
{
	if (UAssassinsAbilitySystemComponent* AbilitySystem = Player.GetClientAbilitySystem())
	{
		FGameplayTag Tag = InputTag;
		AbilitySystem->AbilityInputTagReleased(Tag);
	}
}

void FAbilityTestDriver::AddTapSteps(FAbilityTestSteps& Steps, int32 PlayerIndex, const FGameplayTag& InputTag)
{
	Steps.Do(FString::Printf(TEXT("Press %s"), *InputTag.ToString()), [PlayerIndex, InputTag]
	{
		PressInput(FAbilityTestSession::Get().GetPlayers()[PlayerIndex], InputTag);
	});

	Steps.Do(FString::Printf(TEXT("Release %s"), *InputTag.ToString()), [PlayerIndex, InputTag]
	{
		ReleaseInput(FAbilityTestSession::Get().GetPlayers()[PlayerIndex], InputTag);
	});
}

void FAbilityTestDriver::AddMoveInput(const FAbilityTestPlayer& Player, const FVector& Direction)
{
	if (ACharacter* Champion = Player.ClientChampion.Get())
	{
		Champion->AddMovementInput(Direction.GetSafeNormal2D(), 1.0f, /*bForce*/ false);
	}
}
