// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SubclassOf.h"
#include "CommonPlayerController.h"
#include "GameplayTagContainer.h"
#include "AITypes.h"

#include "AssassinsPlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UInputAction;
class UAssassinsAbilitySystemComponent;
class UPathFollowingComponent;
struct FPathFollowingResult;
struct FPathFollowingRequestResult;

namespace EPathFollowingResult { enum Type : int; }
namespace EPathFollowingRequestResult { enum Type : int; }

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMoveCompletedSignature, FAIRequestID, RequestID, EPathFollowingResult::Type, Result);


UCLASS()
class AAssassinsPlayerController : public ACommonPlayerController
{
	GENERATED_BODY()

public:
	AAssassinsPlayerController();

	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponent() const;

	/** Makes AI go toward specified Goal actor(destination will be continuously updated), aborts any active path following */
	// Me: It's for unit-targeted abilities(including basic attack), which force the avatar to move towards the target until the target is within the fire range.
	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	EPathFollowingRequestResult::Type MoveToActor(AActor* Goal, float AcceptRadius);

    UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
    void AbortMove();

    UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
    void SetShouldKeepMoving(bool Condition) { bShouldKeepMoving = Condition; }

    UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
    bool GetShouldKeepMoving() const { return bShouldKeepMoving; }

	/** Blueprint notification that we've completed the current movement request */
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "MoveCompleted"))
	FMoveCompletedSignature ReceiveMoveCompleted;

protected:	

	//~AController interface
	virtual void OnUnPossess() override;
	//~End of AController interface

	//~APlayerController interface
    virtual void PlayerTick(float DeltaTime) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface

	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

private:
	
	/**
	* Me: Functions of AI component, which are called in MoveToActor.
	*/

	/** Makes AI go toward specified destination */
	FPathFollowingRequestResult MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath = nullptr);

	/** Helper function for creating pathfinding query for this agent from move request data and starting location */
	bool BuildPathfindingQuery(const FAIMoveRequest& MoveRequest, const FVector& StartLocation, FPathFindingQuery& OutQuery) const;

	/** Finds path for given move request */
	void FindPathForMoveRequest(const FAIMoveRequest& MoveRequest, FPathFindingQuery& Query, FNavPathSharedPtr& OutPath) const;

	/** Merges the remaining points of InitialPath, with the points of InOutMergePath. The resulting merged path is outputted into InOutMergePath */
	void MergePaths(const FNavPathSharedPtr& InitialPath, FNavPathSharedPtr& InOutMergedPath) const;

	/** Passes move request and path object to path following */
	FAIRequestID RequestMove(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr Path);

private:

	/** Component used for moving along a path. */
	UPROPERTY(VisibleDefaultsOnly, Category = "AI|Navigation")
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;

    // Me: Whether to keep MoveToActor
    bool bShouldKeepMoving;

    // Me: Arguments to call MoveToActor again
    UPROPERTY()
    TWeakObjectPtr<AActor> CachedMoveTarget;

    float CachedAcceptRadius;
};


