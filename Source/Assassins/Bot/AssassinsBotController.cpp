// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/AssassinsBotController.h"
#include "Character/AssassinsCharacterWithAbilities.h"
#include "Navigation/PathFollowingComponent.h"
#include "AbilitySystem/AssassinsTargetChasingComponent.h"

AAssassinsBotController::AAssassinsBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanSetTeamId = false;

	TargetChasingComponent = CreateDefaultSubobject<UAssassinsTargetChasingComponent>(TEXT("TargetChasingComponent"));
}

void AAssassinsBotController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAssassinsBotController::BeginPlay()
{
	Super::BeginPlay();

	if (TargetChasingComponent)
	{
		TargetChasingComponent->ChaseTargetDelegate.BindLambda([this](AActor* Target, float AcceptRadius)
		{
			MoveToActor(Target, AcceptRadius);
		});

		TargetChasingComponent->StopChaseDelegate.BindLambda([this]() { StopMovement(); });
	}
}

void AAssassinsBotController::SetGenericTeamId(const FGenericTeamId& NewID)
{
	MyTeamID = NewID;

	bCanSetTeamId = true;
	if (AAssassinsCharacterWithAbilities* Bot = GetPawn<AAssassinsCharacterWithAbilities>())
	{
		Bot->SetGenericTeamId(NewID);
	}
	bCanSetTeamId = false;
}

FGenericTeamId AAssassinsBotController::GetGenericTeamId() const
{
	return MyTeamID;
}

void AAssassinsBotController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (TargetChasingComponent)
	{
		TargetChasingComponent->HandleChaseCompleted.Broadcast(RequestID, Result.Code);
	}
}