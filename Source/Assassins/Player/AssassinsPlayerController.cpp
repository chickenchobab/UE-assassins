// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/AssassinsPlayerController.h"
#include "Player/AssassinsPlayerState.h"
#include "GameFramework/Pawn.h"
#include "Character/AssassinsCharacter.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "AssassinsLogCategories.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AAssassinsPlayerController::AAssassinsPlayerController()
{
	PathFollowingComponent = CreateDefaultSubobject<UPathFollowingComponent>(TEXT("PathFollowingComponent"));
	if (PathFollowingComponent)
	{
		PathFollowingComponent->OnRequestFinished.AddUObject(this, &ThisClass::OnMoveCompleted);
	}

    bShouldKeepMoving = false;
    CachedMoveTarget = nullptr;
    CachedAcceptRadius = 0.0f;
}

UAssassinsAbilitySystemComponent* AAssassinsPlayerController::GetAssassinsAbilitySystemComponent() const
{
	const AAssassinsPlayerState* AssassinsPS = GetPlayerState<AAssassinsPlayerState>();
	return AssassinsPS ? AssassinsPS->GetAssassinsAbilitySystemComponent() : nullptr;
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

	Super::OnUnPossess();
}

void AAssassinsPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (bShouldKeepMoving && PathFollowingComponent->GetStatus() == EPathFollowingStatus::Idle)
    {
        MoveToActor(CachedMoveTarget.IsValid() ? CachedMoveTarget.Get() : nullptr, CachedAcceptRadius);
    }
}

void AAssassinsPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponent())
	{
		AssassinsASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AAssassinsPlayerController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    ReceiveMoveCompleted.Broadcast(RequestID, Result.Code);
}

EPathFollowingRequestResult::Type AAssassinsPlayerController::MoveToActor(AActor* Goal, float AcceptRadius)
{
    AbortMove();

    CachedMoveTarget = Goal;
    CachedAcceptRadius = AcceptRadius;

	FAIMoveRequest MoveReq(Goal);
	MoveReq.SetAcceptanceRadius(AcceptRadius);

	MoveReq.SetUsePathfinding(true);
	MoveReq.SetAllowPartialPath(true);
	MoveReq.SetReachTestIncludesAgentRadius(true);
	MoveReq.SetCanStrafe(false); // Me: No side walk

	return MoveTo(MoveReq);
}

void AAssassinsPlayerController::AbortMove()
{
    if (PathFollowingComponent && PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
    {
        PathFollowingComponent->AbortMove(*this, FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest, FAIRequestID::CurrentRequest, EPathFollowingVelocityMode::Keep);
    }
}

void AAssassinsPlayerController::PauseMove()
{
    PathFollowingComponent->PauseMove();
}

void AAssassinsPlayerController::ResetMoveState()
{
    ReceiveMoveCompleted.Clear();

    bShouldKeepMoving = false;
    CachedMoveTarget = nullptr;
    CachedAcceptRadius = 0.0f;
}

void AAssassinsPlayerController::HandleBeginChanneling()
{
    PathFollowingComponent->PauseMove();
}

void AAssassinsPlayerController::HandleEndChanneling(bool bResumeMove)
{
    // Me: Existing move request could have been aborted by a new request so check the status
    if (bResumeMove && PathFollowingComponent && PathFollowingComponent->GetStatus() == EPathFollowingStatus::Paused)
    {
        PathFollowingComponent->ResumeMove();
    }
}

bool AAssassinsPlayerController::HasMovePaused() const
{
    return (PathFollowingComponent && PathFollowingComponent->GetStatus() == EPathFollowingStatus::Paused);
}

FPathFollowingRequestResult AAssassinsPlayerController::MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath)
{
	FPathFollowingRequestResult ResultData;
	ResultData.Code = EPathFollowingRequestResult::Failed;

	if (!MoveRequest.IsValid())
	{
		return ResultData;
	}

	if (PathFollowingComponent == nullptr)
	{
		return ResultData;
	}

	bool bCanRequestMove = true;
	bool bAlreadyAtGoal = bCanRequestMove && PathFollowingComponent->HasReached(MoveRequest);

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

			const FAIRequestID RequestID = Path.IsValid() ? RequestMove(MoveRequest, Path) : FAIRequestID::InvalidRequest;
			if (RequestID.IsValid())
			{
				ResultData.MoveId = RequestID;
				ResultData.Code = EPathFollowingRequestResult::RequestSuccessful;

				if (OutPath)
				{
					*OutPath = Path;
				}
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
	const ANavigationData* NavData = (NavSys == nullptr) ? nullptr : 
		(MoveRequest.IsUsingPathfinding() ? NavSys->GetNavDataForProps(GetNavAgentPropertiesRef(), GetNavAgentLocation()) : NavSys->GetAbstractNavData());

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
			UE_VLOG(this, LogAssassinsAINavigation, Warning, TEXT("Unable AssassinsPlayerController::BuildPathfindingQuery due to no NavigationSystem present. Note that even pathfinding-less movement requires presence of NavigationSystem."));
		}
		else
		{
			UE_VLOG(this, LogAssassinsAINavigation, Warning, TEXT("Unable to find NavigationData instance while calling AssassinsPlayerController::BuildPathfindingQuery"));
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
	}
	else
	{
		UE_VLOG(this, LogAssassinsAINavigation, Error, TEXT("Trying to find path to %s resulted in Error")
			, MoveRequest.IsMoveToActorRequest() ? *GetNameSafe(MoveRequest.GetGoalActor()) : *MoveRequest.GetGoalLocation().ToString());
		UE_VLOG_SEGMENT(this, LogAssassinsAINavigation, Error, GetPawn() ? GetPawn()->GetActorLocation() : FAISystem::InvalidLocation
			, MoveRequest.GetGoalLocation(), FColor::Red, TEXT("Failed move to %s"), *GetNameSafe(MoveRequest.GetGoalActor()));
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