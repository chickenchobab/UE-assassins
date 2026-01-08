// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/AssassinsBotController.h"
#include "Character/AssassinsCharacterWithAbilities.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "AbilitySystem/AssassinsTargetChasingComponent.h"

AAssassinsBotController::AAssassinsBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	bCanSetTeamId = false;

	TargetChasingComponent = CreateDefaultSubobject<UAssassinsTargetChasingComponent>(TEXT("TargetChasingComponent"));

	CollisionQueryRange = 300.0f;
	SeparationWeight = 2.0f;
}

void AAssassinsBotController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAssassinsBotController::BeginPlay()
{
	Super::BeginPlay();

	if (UCrowdFollowingComponent* CrowdFollowingComponent = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		CrowdFollowingComponent->SetCrowdSimulationState(ECrowdSimulationState::Enabled);
		CrowdFollowingComponent->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
		CrowdFollowingComponent->SetCrowdCollisionQueryRange(CollisionQueryRange);
		CrowdFollowingComponent->SetCrowdSeparation(true);
		CrowdFollowingComponent->SetCrowdSeparationWeight(SeparationWeight);
	}

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

	if (UCrowdFollowingComponent* CrowdFollowingComponent = Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
	{
		int32 AvoidanceGroup = GenericTeamIdToInteger(NewID);
		CrowdFollowingComponent->SetAvoidanceGroup(1 << AvoidanceGroup);
		CrowdFollowingComponent->SetGroupsToAvoid(1 << AvoidanceGroup);
	}
}

FGenericTeamId AAssassinsBotController::GetGenericTeamId() const
{
	return MyTeamID;
}

void AAssassinsBotController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (TargetChasingComponent)
	{
		TargetChasingComponent->HandleChaseCompleted.Broadcast(RequestID, Result.Code);
	}
}