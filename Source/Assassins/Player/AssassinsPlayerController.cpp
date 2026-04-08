// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/AssassinsPlayerController.h"
#include "Player/AssassinsPlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/AssassinsTargetChasingComponent.h"
#include "AbilitySystemGlobals.h"
#include "NavigationSystem.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Navigation/PathFollowingComponent.h"
#include "AssassinsLogCategories.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AAssassinsPlayerController::AAssassinsPlayerController()
{
	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("PathFollowingComponent"));

	TargetChasingComponent = CreateDefaultSubobject<UAssassinsTargetChasingComponent>(TEXT("TargetChasingComponent"));
}

UAssassinsAbilitySystemComponent* AAssassinsPlayerController::GetAssassinsAbilitySystemComponent() const
{
	const AAssassinsPlayerState* AssassinsPS = GetPlayerState<AAssassinsPlayerState>();
	return AssassinsPS ? AssassinsPS->GetAssassinsAbilitySystemComponent() : nullptr;
}

void AAssassinsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (PathFollowingComponent)
	{
		PathFollowingComponent->OnRequestFinished.AddUObject(this, &ThisClass::OnMoveCompleted);
	}

	if (TargetChasingComponent)
	{
		TargetChasingComponent->ChaseTargetDelegate.BindLambda([this](AActor* Target, float AcceptRadius)
		{
			if (PathFollowingComponent && PathFollowingComponent->GetStatus() == EPathFollowingStatus::Idle)
			{
				MoveToActor(Target, AcceptRadius);
			}
		});

		TargetChasingComponent->StopChaseDelegate.BindLambda([this]() { StopMovement(); });
	}
}

void AAssassinsPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (PathFollowingComponent)
	{
		PathFollowingComponent->Initialize();
	}
}

void AAssassinsPlayerController::OnUnPossess()
{
	// Make sure the pawn that is being unpossessed doesn't remain our ASC's avatar actor
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	if (PathFollowingComponent)
	{
		PathFollowingComponent->Cleanup();
	}
	
	PathFollowingComponent = nullptr;

	Super::OnUnPossess();
}

void AAssassinsPlayerController::StopMovement()
{
	ResetMoveState();

	SetMoveRequestOfCMC(FMoveRequestForReachTest(), false);

	// AbortMove is called
	Super::StopMovement();
}

void AAssassinsPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// When we're a client connected to a remote server, the player controller may replicate later than the player state and ability system component.
	// However, TryActivateAbilitiesOnSpawn depends on the player controller being replicated in order to check whether on-spawn abilities should
	// execute locally. Therefore once the player controller exists and has resolved the player state, try once again to activate on-spawn abilities.
	// The handling here is only for when the PS ans ASC replicated before the PC and incorrect thought the abilities were not for the local player.
	if (GetWorld()->IsNetMode(NM_Client))
	{
		if (AAssassinsPlayerState* AssassinsPS = GetPlayerState<AAssassinsPlayerState>())
		{
			if (UAssassinsAbilitySystemComponent* AssassinsASC = AssassinsPS->GetAssassinsAbilitySystemComponent())
			{
				AssassinsASC->RefreshAbilityActorInfo();
				AssassinsASC->TryActivateAbilitiesOnSpawn();
			}
		}
	}
}

void AAssassinsPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
}

void AAssassinsPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponent())
	{
		AssassinsASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AAssassinsPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogAssassinsTeams, Error, TEXT("You can't set the team ID on a player controller (%s); it's deriven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AAssassinsPlayerController::GetGenericTeamId() const
{
	if (const IAssassinsTeamAgentInterface* PSWithTeamInterface = Cast<IAssassinsTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

void AAssassinsPlayerController::PauseMove()
{
	if (PathFollowingComponent)
	{
		PathFollowingComponent->PauseMove();
	}
}

void AAssassinsPlayerController::ResumeMove() 
{
	if (PathFollowingComponent)
	{
		PathFollowingComponent->ResumeMove();
	}
}

void AAssassinsPlayerController::ResetMoveState()
{
    ReceiveMoveCompleted.Clear();
	
	if (TargetChasingComponent)
	{
		TargetChasingComponent->ResetTargetState();
	}
}

bool AAssassinsPlayerController::HasMovePaused() const
{
    return (PathFollowingComponent && PathFollowingComponent->GetStatus() == EPathFollowingStatus::Paused);
}

bool AAssassinsPlayerController::HasMoveReached(const FMoveRequestForReachTest& RequestForReachTest)
{
	if (PathFollowingComponent)
	{
		return PathFollowingComponent->HasReached(FromReachTestRequest(RequestForReachTest));
	}

	return true;
}

void AAssassinsPlayerController::SetAvoidanceGroup(int32 AvoidanceGroup)
{
	if (APawn* PlayerPawn = GetPawn())
	{
		if (UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(PlayerPawn->GetMovementComponent()))
		{
			CharacterMovement->SetAvoidanceGroupMask(1 << AvoidanceGroup);
		}
	}
}

void AAssassinsPlayerController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	ReceiveMoveCompleted.Broadcast(RequestID, Result.Code);

	if (TargetChasingComponent)
	{
		TargetChasingComponent->HandleChaseCompleted.Broadcast(RequestID, Result.Code);
	}

	UE_LOG(LogTemp, Display, TEXT("Move finished: %s"), *UEnum::GetValueAsString(Result.Code));
	SetMoveRequestOfCMC(FMoveRequestForReachTest(), false);
}

FAIMoveRequest AAssassinsPlayerController::FromReachTestRequest(const FMoveRequestForReachTest& ReachTestRequest)
{
	FAIMoveRequest MoveReq;

	if (ReachTestRequest.bMoveToActor)
	{
		MoveReq.SetGoalActor(ReachTestRequest.GoalActor);
	}
	else
	{
		MoveReq.SetGoalLocation(ReachTestRequest.GoalLocation);
	}

	MoveReq.SetAcceptanceRadius(ReachTestRequest.AcceptanceRadius);
	MoveReq.SetReachTestIncludesAgentRadius(ReachTestRequest.bReachTestIncludesAgentRadius);
	MoveReq.SetReachTestIncludesGoalRadius(ReachTestRequest.bReachTestIncludesGoalRadius);

	return MoveReq;
}

FMoveRequestForReachTest AAssassinsPlayerController::ToReachTestRequest(const FAIMoveRequest& Request)
{
	FMoveRequestForReachTest ReachTestRequest;
	if (!Request.IsValid())
	{
		return ReachTestRequest;
	}

	ReachTestRequest.bMoveToActor = Request.IsMoveToActorRequest();
	if (ReachTestRequest.bMoveToActor)
	{
		ReachTestRequest.GoalActor = Request.GetGoalActor();
	}
	else
	{
		ReachTestRequest.GoalLocation = Request.GetGoalLocation();
	}

	ReachTestRequest.AcceptanceRadius = Request.GetAcceptanceRadius();
	ReachTestRequest.bReachTestIncludesAgentRadius = Request.IsReachTestIncludingAgentRadius();
	ReachTestRequest.bReachTestIncludesGoalRadius = Request.IsReachTestIncludingGoalRadius();

	return ReachTestRequest;
}

EPathFollowingRequestResult::Type AAssassinsPlayerController::MoveToActor(AActor* Goal, float AcceptRadius)
{
	// Abort active movement to keep only one request running
	if (PathFollowingComponent && PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
	{
		PathFollowingComponent->AbortMove(*this, FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest
			, FAIRequestID::CurrentRequest, EPathFollowingVelocityMode::Keep);

		SetMoveRequestOfCMC(FMoveRequestForReachTest(), false);
	}

	FAIMoveRequest MoveReq(Goal);
	MoveReq.SetAcceptanceRadius(AcceptRadius);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetReachTestIncludesAgentRadius(false);
	MoveReq.SetCanStrafe(false); // Me: No side walk

	return MoveTo(MoveReq);
}

EPathFollowingRequestResult::Type AAssassinsPlayerController::MoveToLocation(const FVector& Dest, float AcceptanceRadius)
{
	// Abort active movement to keep only one request running
	if (PathFollowingComponent && PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
	{
		PathFollowingComponent->AbortMove(*this, FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest
			, FAIRequestID::CurrentRequest, EPathFollowingVelocityMode::Keep);

		SetMoveRequestOfCMC(FMoveRequestForReachTest(), false);
	}

	FAIMoveRequest MoveReq(Dest);
	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetProjectGoalLocation(true);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);
	MoveReq.SetReachTestIncludesAgentRadius(true);
	MoveReq.SetCanStrafe(false);

	return MoveTo(MoveReq);
}

FPathFollowingRequestResult AAssassinsPlayerController::MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath)
{
	FPathFollowingRequestResult ResultData;
	ResultData.Code = EPathFollowingRequestResult::Failed;

	if (!MoveRequest.IsValid())
	{
		UE_VLOG(this, LogAssassinsAINavigation, Error, TEXT("MoveTo request failed due to MoveRequest not being valid. Most probably desired goal actor no loger exists. MoveRequest : '%s'"), *MoveRequest.ToString());
		return ResultData;
	}

	if (PathFollowingComponent == nullptr)
	{
		UE_VLOG(this, LogAssassinsAINavigation, Error, TEXT("MoveTo request failed due to missing PathFollowingComponent"));
		return ResultData;
	}

	bool bCanRequestMove = true;
	bool bAlreadyAtGoal = false;

	if (!MoveRequest.IsMoveToActorRequest())
	{
		if (MoveRequest.GetGoalLocation().ContainsNaN() || FAISystem::IsValidLocation(MoveRequest.GetGoalLocation()) == false)
		{
			UE_VLOG(this, LogAssassinsAINavigation, Error, TEXT("AAssassinsPlayerController::MoveTo: Destination is not valid! Goal(%s)"), TEXT_AI_LOCATION(MoveRequest.GetGoalLocation()));
			bCanRequestMove = false;
		}

		// Fail if projection to navigation is required but it failed
		if (bCanRequestMove && MoveRequest.IsProjectingGoal())
		{
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			const FNavAgentProperties& AgentProps = GetNavAgentPropertiesRef();
			FNavLocation ProjectedLocation;

			if (NavSys && !NavSys->ProjectPointToNavigation(MoveRequest.GetGoalLocation(), ProjectedLocation, INVALID_NAVEXTENT, &AgentProps))
			{
				UE_VLOG_LOCATION(this, LogAssassinsAINavigation, Error, MoveRequest.GetGoalLocation(), 30.f, FColor::Red, TEXT("AAssassinsPlayerController::MoveTo failed to project destination location to navmesh"));
				bCanRequestMove = false;
			}

			MoveRequest.UpdateGoalLocation(ProjectedLocation.Location);
		}

	}
	
	bAlreadyAtGoal = bCanRequestMove && PathFollowingComponent->HasReached(MoveRequest);

	if (bAlreadyAtGoal)
	{
		UE_VLOG(this, LogAssassinsAINavigation, Log, TEXT("MoveTo: already at goal!"));
		ResultData.MoveId = PathFollowingComponent->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
		ResultData.Code = EPathFollowingRequestResult::AlreadyAtGoal;
	}
	else if (bCanRequestMove)
	{
		FPathFindingQuery PFQuery;

		bool bShouldMergePaths = false;
		FVector StartLocation = GetNavAgentLocation();
		if (MoveRequest.ShouldStartFromPreviousPath())
		{
			FNavPathSharedPtr CurrentPath = PathFollowingComponent->GetPath();
			if (CurrentPath.IsValid() && CurrentPath->IsValid() && CurrentPath->GetPathPoints().Num() > 0)
			{
				StartLocation = CurrentPath->GetPathPoints().Last();
				bShouldMergePaths = true;
			}
		}

		const bool bValidQuery = BuildPathfindingQuery(MoveRequest, StartLocation, PFQuery);
		if (bValidQuery)
		{
			FNavPathSharedPtr Path;
			FindPathForMoveRequest(MoveRequest, PFQuery, Path);

			if (bShouldMergePaths && Path.IsValid())
			{
				// Merge the newly generated path with the current one
				MergePaths(PathFollowingComponent->GetPath(), Path);
			}

			FMoveRequestForReachTest ReachTestRequest = ToReachTestRequest(MoveRequest);

			const FAIRequestID RequestID = Path.IsValid() ? RequestMove(MoveRequest, Path) : FAIRequestID::InvalidRequest;
			if (RequestID.IsValid())
			{
				ResultData.MoveId = RequestID;
				ResultData.Code = EPathFollowingRequestResult::RequestSuccessful;

				if (OutPath)
				{
					*OutPath = Path;
				}

				SetMoveRequestOfCMC(ReachTestRequest);
			}
		}
	}

	if (ResultData.Code == EPathFollowingRequestResult::Failed)
	{
		ResultData.MoveId = PathFollowingComponent->RequestMoveWithImmediateFinish(EPathFollowingResult::Invalid);
	}

	return ResultData;
}

bool AAssassinsPlayerController::BuildPathfindingQuery(const FAIMoveRequest& MoveRequest, const FVector& StartLocation, FPathFindingQuery& OutQuery) const
{
	bool bResult = false;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const ANavigationData* NavData = (NavSys == nullptr) ? nullptr : (MoveRequest.IsUsingPathfinding() ? NavSys->GetNavDataForProps(GetNavAgentPropertiesRef(), GetNavAgentLocation()) : NavSys->GetAbstractNavData());

	if (NavData)
	{
		FVector GoalLocation = MoveRequest.GetGoalLocation();
		if (MoveRequest.IsMoveToActorRequest())
		{
			const INavAgentInterface* NavGoal = Cast<const INavAgentInterface>(MoveRequest.GetGoalActor());
			if (NavGoal)
			{
				const FVector Offset = NavGoal->GetMoveGoalOffset(this);
				GoalLocation = FQuatRotationTranslationMatrix(MoveRequest.GetGoalActor()->GetActorQuat(), NavGoal->GetNavAgentLocation()).TransformPosition(Offset);
			}
			else
			{
				GoalLocation = MoveRequest.GetGoalActor()->GetActorLocation();
			}
		}

		FSharedConstNavQueryFilter NavFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, this, MoveRequest.GetNavigationFilter());
		OutQuery = FPathFindingQuery(*this, *NavData, StartLocation, GoalLocation, NavFilter);
		OutQuery.SetAllowPartialPaths(MoveRequest.IsUsingPartialPaths());
		OutQuery.SetRequireNavigableEndLocation(MoveRequest.IsNavigableEndLocationRequired());
		if (MoveRequest.IsApplyingCostLimitFromHeuristic())
		{
			const float HeuristicScale = NavFilter->GetHeuristicScale();
			OutQuery.CostLimit = FPathFindingQuery::ComputeCostLimitFromHeuristic(OutQuery.StartLocation, OutQuery.EndLocation, HeuristicScale, MoveRequest.GetCostLimitFactor(), MoveRequest.GetMinimumCostLimit());
		}

		if (PathFollowingComponent)
		{
			PathFollowingComponent->OnPathfindingQuery(OutQuery);
		}

		bResult = true;
	}
	else
	{
		if (NavSys == nullptr)
		{
			UE_VLOG(this, LogAssassinsAINavigation, Warning, TEXT("Unable AAssassinsPlayerController::BuildPathfindingQuery due to no NavigationSystem present. Note that even pathfinding-less movement requires presence of NavigationSystem."));
		}
		else
		{
			UE_VLOG(this, LogAssassinsAINavigation, Warning, TEXT("Unable to find NavigationData instance while calling AAssassinsPlayerController::BuildPathfindingQuery"));
		}
	}

	return bResult;
}

void AAssassinsPlayerController::FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FPathFindingResult PathResult = NavSys->FindPathSync(Query);
		if (PathResult.Result != ENavigationQueryResult::Error)
		{
			if (PathResult.IsSuccessful() && PathResult.Path.IsValid())
			{
				PathResult.Path->SetGoalActorObservation(*MoveRequest.GetGoalActor(), 100.0f);
			}

			PathResult.Path->EnableRecalculationOnInvalidation(true);
			OutPath = PathResult.Path;
		}
		else
		{
			UE_VLOG(this, LogAssassinsAINavigation, Error, TEXT("Trying to find path to %s resulted in Error"), MoveRequest.IsMoveToActorRequest() ? *GetNameSafe(MoveRequest.GetGoalActor()) : *MoveRequest.GetGoalLocation().ToString());
			UE_VLOG_SEGMENT(this, LogAssassinsAINavigation, Error, GetPawn() ? GetPawn()->GetActorLocation() : FAISystem::InvalidLocation, MoveRequest.GetGoalLocation(), FColor::Red, TEXT("Failed move to %s"), *GetNameSafe(MoveRequest.GetGoalActor()));
		}
	}
}

void AAssassinsPlayerController::MergePaths(const FNavPathSharedPtr& InitialPath, FNavPathSharedPtr& InOutMergedPath) const
{
	if (!InitialPath.IsValid() || !InitialPath->IsValid())
	{
		UE_VLOG_UELOG(this, LogAssassinsAINavigation, Error, TEXT("%hs: InitialPath is Invalid"), __FUNCTION__);
		return;
	}

	if (!InOutMergedPath.IsValid() || !InOutMergedPath->IsValid())
	{
		UE_VLOG_UELOG(this, LogAssassinsAINavigation, Error, TEXT("%hs: InOutMergedPath is Invalid"), __FUNCTION__);
		return;
	}

	const TArray<FNavPathPoint>& InitialPathPoints = InitialPath->GetPathPoints();
	TArray<FNavPathPoint>& InOutPathPoints = InOutMergedPath->GetPathPoints();

	if (!InitialPathPoints.Last().Location.Equals(InOutPathPoints[0].Location))
	{
		UE_VLOG_UELOG(this, LogAssassinsAINavigation, Error, TEXT("%hs: last %s and first %s points don't match."), __FUNCTION__, *InitialPathPoints.Last().Location.ToString(), *InOutPathPoints[0].Location.ToString());
		return;
	}

	// We don't want to keep path points that have already been traversed, so only merge the points starting from "CurrentPathIndex".
	const int32 StartingPointIndex = PathFollowingComponent ? PathFollowingComponent->GetCurrentPathIndex() : 0;
	if (StartingPointIndex < InitialPathPoints.Num())
	{
		InOutPathPoints.Insert(&InitialPathPoints[StartingPointIndex], InitialPathPoints.Num() - StartingPointIndex - 1, 0);
	}
}

FAIRequestID AAssassinsPlayerController::RequestMove(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr Path)
{
	uint32 RequestID = FAIRequestID::InvalidRequest;
	if (PathFollowingComponent)
	{
		RequestID = PathFollowingComponent->RequestMove(MoveRequest, Path);
	}
	
	return RequestID;
}

void AAssassinsPlayerController::NotifyMoveSuccess()
{
	ReceiveMoveCompleted.Broadcast(FAIRequestID(), EPathFollowingResult::Success);

	if (TargetChasingComponent)
	{
		TargetChasingComponent->HandleChaseCompleted.Broadcast(FAIRequestID(), EPathFollowingResult::Success);
	}
}

void AAssassinsPlayerController::SetMoveRequestOfCMC(FMoveRequestForReachTest ReachTestRequest, bool bShouldInitialize)
{
	if (GetNetMode() != NM_Client)
	{
		return;
	}

	if (bShouldInitialize)
	{
		ReachTestRequest.Init(GetWorld()->GetTimeSeconds());
	}

	if (ACharacter* MyCharacter = GetCharacter())
	{
		if (UAssassinsCharacterMovementComponent* AssassinsCMC = Cast<UAssassinsCharacterMovementComponent>(MyCharacter->GetCharacterMovement()))
		{
			AssassinsCMC->SetMoveRequest(ReachTestRequest);
		}
	}
}
