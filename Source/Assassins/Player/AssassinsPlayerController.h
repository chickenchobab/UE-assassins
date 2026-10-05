// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SubclassOf.h"
#include "CommonPlayerController.h"
#include "Teams/AssassinsTeamAgentInterface.h"
#include "GameplayTagContainer.h"
#include "AITypes.h"

#include "AssassinsPlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UInputAction;
class UAssassinsAbilitySystemComponent;
class UAssassinsTargetChasingComponent;
class UPathFollowingComponent;
struct FPathFollowingResult;
struct FPathFollowingRequestResult;

namespace EPathFollowingResult { enum Type : int; }
namespace EPathFollowingRequestResult { enum Type : int; }

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMoveCompletedSignature, FAIRequestID, RequestID, EPathFollowingResult::Type, Result);
DECLARE_MULTICAST_DELEGATE_OneParam(FPlayerRestartedDelegate, ACharacter*);

UCLASS()
class ASSASSINS_API AAssassinsPlayerController : public ACommonPlayerController, public IAssassinsTeamAgentInterface
{
	GENERATED_BODY()

public:
	AAssassinsPlayerController();

	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponent() const;

	//~Actor interface
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	//~End of Actor interface

	//~AController interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void StopMovement() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

	//~APlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface

	//~IAssassinsTeamAgentInterface interface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~End of IAssassinsTeamAgentInterface interface

	FORCEINLINE void SetPlayerRestarted(bool bRestarted) { bPlayerRestarted = bRestarted; }
	FORCEINLINE bool GetPlayerRestarted() { return bPlayerRestarted; }

	UFUNCTION(Server, Reliable)
	void Server_MoveToActor(AActor* Goal, float AcceptRadius);
	UFUNCTION(Server, Reliable)
	void Server_MoveToLocation(const FVector& Dest, float AcceptanceRadius = -1);

	UFUNCTION(Server, Reliable)
	void Server_PauseMove();
	UFUNCTION(Server, Reliable)
	void Server_ResumeMove();

	UFUNCTION(Server, Reliable)
	void Server_StopMovement();

	/** Makes AI go toward specified Goal actor(destination will be continuously updated), aborts any active path following */
	EPathFollowingRequestResult::Type MoveToActor(AActor* Goal, float AcceptRadius);
	/** Makes AI go toward specified Dest location, aborts any active path following */
	EPathFollowingRequestResult::Type MoveToLocation(const FVector& Dest, float AcceptanceRadius = -1);

    UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
    void PauseMove();
	UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
	void ResumeMove();

    UFUNCTION(BlueprintCallable, Category = "AI|Navigation")
    void ResetMoveState();

	UFUNCTION(BlueprintPure, Category = "AI|Navigation")
	bool HasMovePaused() const;

	void NotifyMoveSuccess();

	void SetAvoidanceGroup(int32 AvoidanceGroup);

#if !UE_BUILD_SHIPPING
	// Automation: the ability inputs of this player aim at this location and target(may be null) in place of the mouse,
	// until cleared. Only the automation tests set it.
	void SetAimOverride(const FVector& Location, AActor* Target);
	void ClearAimOverride();

	// Whether the automation aims for this player. If so, fills what the ability inputs would otherwise take from under
	// the cursor(the hit) and from the hero component(the target).
	bool GetAimOverride(FHitResult& OutHitResult, AActor*& OutTarget) const;
#endif

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UAssassinsTargetChasingComponent> TargetChasingComponent;

	FPlayerRestartedDelegate OnPlayerRestarted;

protected:

	void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
	
	///////////////////////////////////////////////////////////////
	// AI controller functions for MoveToActor and MoveToLocation
	///////////////////////////////////////////////////////////////

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

	/** Blueprint notification that we've completed the current movement request */
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "MoveCompleted"))
	FMoveCompletedSignature ReceiveMoveCompleted;

	bool bPlayerRestarted : 1;

#if !UE_BUILD_SHIPPING
	TOptional<FVector> AimOverrideLocation;
	TWeakObjectPtr<AActor> AimOverrideTarget;
#endif
};