// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/AssassinsCharacter.h"
#include "Character/Movements/AssassinsRootMotionSource.h"
#include "Player/AssassinsPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "NativeGameplayTags.h"
#include "AssassinsLogCategories.h"

namespace MovementStatus
{
	// Keeps the abilities a dash holds back from starting. The dash task puts it on, and takes it off as it ends.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_DASHING, "Status.Dashing");
};

UAssassinsCharacterMovementComponent::UAssassinsCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsDashing = 0;

	bWantsToTeleport = 0;
	TeleportLocation = FVector::ZeroVector;
	TeleportRotation = FRotator::ZeroRotator;

	SetNetworkMoveDataContainer(AssassinsNetworkMoveDataContainer);
	SetMoveResponseDataContainer(AssassinsMoveResponseDataContainer);
}

void UAssassinsCharacterMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel)
{
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

void UAssassinsCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Dashing:
		PhysDashing(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogAssassins, Error, TEXT("Invalid movement mode"));
	}
}

void UAssassinsCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	if (bWantsToTeleport)
	{
		UpdatedComponent->SetWorldLocationAndRotation(TeleportLocation, TeleportRotation, false, nullptr, ETeleportType::ResetPhysics);

		bWantsToTeleport = false;
	}
}

FNetworkPredictionData_Client* UAssassinsCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UAssassinsCharacterMovementComponent* MutableThis = const_cast<UAssassinsCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_AssassinsCharacter(*this);
	}

	return ClientPredictionData;
}

void UAssassinsCharacterMovementComponent::ServerMove_HandleMoveData(const FCharacterNetworkMoveDataContainer& MoveDataContainer)
{
	Super::ServerMove_HandleMoveData(MoveDataContainer);
}

void UAssassinsCharacterMovementComponent::ServerMove_PerformMovement(const FCharacterNetworkMoveData& MoveData)
{
	const FAssassinsCharacterNetworkMoveData& MyMoveData = static_cast<const FAssassinsCharacterNetworkMoveData&>(MoveData);

	bWantsToTeleport = MyMoveData.bWantsToTeleport;
	if (bWantsToTeleport)
	{
		TeleportLocation = MyMoveData.TeleportLocation;
		TeleportRotation = MyMoveData.TeleportRotation;
	}

	Super::ServerMove_PerformMovement(MoveData);
}

void UAssassinsCharacterMovementComponent::ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse)
{
	const FAssassinsCharacterMoveResponseDataContainer& AssassinsMoveResponse = static_cast<const FAssassinsCharacterMoveResponseDataContainer&>(MoveResponse);

	Super::ClientHandleMoveResponse(MoveResponse);
}

void UAssassinsCharacterMovementComponent::ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, TOptional<FRotator> OptionalRotation)
{
	Super::ClientAdjustPosition_Implementation(TimeStamp, NewLoc, NewVel, NewBase, NewBaseBoneName, bHasBase, bBaseRelativePosition, ServerMovementMode, OptionalRotation);
}

bool UAssassinsCharacterMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
	// A teleport asked for since the last move(TeleportCharacter) is in no saved move yet. Replaying the saved moves after
	// a correction of the server sets the flag from each of them(PrepMoveFor), which would drop the request: the teleport
	// would happen neither here nor on the server, which takes it from the moves.
	const bool bPendingTeleport = bWantsToTeleport;
	const FVector PendingTeleportLocation = TeleportLocation;
	const FRotator PendingTeleportRotation = TeleportRotation;

	const bool bResult = Super::ClientUpdatePositionAfterServerUpdate();

	if (bPendingTeleport)
	{
		bWantsToTeleport = true;
		TeleportLocation = PendingTeleportLocation;
		TeleportRotation = PendingTeleportRotation;
	}

	return bResult;
}

void UAssassinsCharacterMovementComponent::DisableMovement()
{
	CachedMovementMode = MovementMode;
	CachedCustomMovementMode = CustomMovementMode;

	Super::DisableMovement();
}

void UAssassinsCharacterMovementComponent::EnableMovement()
{
	if (CharacterOwner)
	{
		SetMovementMode(CachedMovementMode);
	}
	else
	{
		MovementMode = CachedMovementMode;
		CustomMovementMode = CachedCustomMovementMode;
	}
}

void UAssassinsCharacterMovementComponent::PhysDashing(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	// The mode lasts as long as the root motion of the dash. Once it is gone(it finished on the move that reached the
	// target, or the dash was stopped), the move goes on walking: on that very move, the same on every side that simulates
	// it. A correction of the server, which brings over the mode of the server, may also leave the mode to a client whose
	// dash is over, with nothing else to take it back: and in this mode only root motion moves the character.
	if (!CurrentRootMotion.HasOverrideVelocity())
	{
		// The end of the dash for the character too(world static blocking it again), on this same move rather than when
		// the dash task ends, which the server may only see after more moves of the client.
		EndDashingStatus();
		if (AAssassinsCharacter* AssassinsCharacter = Cast<AAssassinsCharacter>(CharacterOwner))
		{
			AssassinsCharacter->FinishDashMovement();
		}

		SetMovementMode(MOVE_Walking);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	ApplyRootMotionToVelocity(deltaTime);

	Iterations++;
	bJustTeleported = false; // Me: Can be modified in SafeMoveUpdateComponent, by ResolvePenetration

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (!HasAnimRootMotion() && !bJustTeleported)
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	// The dash reached its target on this move. What it held back may start right after it: on the server, an ability
	// the owning client pressed as it landed may come in with this move, before the dash task's next tick(Migration/
	// ISSUES.md 103). The task still tells the ability the dash is over on its tick.
	if (!HasUnfinishedDashRootMotion())
	{
		EndDashingStatus();
	}
}

bool UAssassinsCharacterMovementComponent::HasUnfinishedDashRootMotion() const
{
	for (const TSharedPtr<FRootMotionSource>& RootMotionSource : CurrentRootMotion.RootMotionSources)
	{
		if (RootMotionSource.IsValid() && (RootMotionSource->GetScriptStruct() == FRootMotionSource_MoveToDynamicConstantSpeed::StaticStruct())
			&& !RootMotionSource->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
		{
			return true;
		}
	}
	return false;
}

void UAssassinsCharacterMovementComponent::EndDashingStatus()
{
	if ((CharacterOwner == nullptr) || CharacterOwner->bClientUpdating)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CharacterOwner))
	{
		ASC->SetLooseGameplayTagCount(MovementStatus::TAG_STATUS_DASHING, 0);
	}
}

void UAssassinsCharacterMovementComponent::TeleportCharacter(FVector GoalLocation, FRotator GoalRotation)
{
	FVector Adjusted;
	if (GetWorld()->EncroachingBlockingGeometry(CharacterOwner, GoalLocation, GoalRotation, &Adjusted))
	{
		GoalLocation = Adjusted;
	}

	if (GoalLocation == FVector(0.f))
	{
		return;
	}

	if (CharacterOwner->HasAuthority())
	{
		CharacterOwner->SetActorLocationAndRotation(GoalLocation, GoalRotation, false, nullptr, ETeleportType::ResetPhysics);
		return;
	}

	bWantsToTeleport = true;
	TeleportLocation = GoalLocation;
	TeleportRotation = GoalRotation;
}

///////////////////////////////////////////////////////////////////
// Saved move
///////////////////////////////////////////////////////////////////

FSavedMove_AssassinsCharacter::FSavedMove_AssassinsCharacter()
{
	bIsDashing = 0;

	bWantsToTeleport = 0;
	TeleportLocation = FVector::ZeroVector;
	TeleportRotation = FRotator::ZeroRotator;
}

void FSavedMove_AssassinsCharacter::Clear()
{
	Super::Clear();

	bIsDashing = 0;

	bWantsToTeleport = 0;
	TeleportLocation = FVector::ZeroVector;
	TeleportRotation = FRotator::ZeroRotator;
}

void FSavedMove_AssassinsCharacter::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const UAssassinsCharacterMovementComponent* AssassinsCMC = Cast<UAssassinsCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		bIsDashing = AssassinsCMC->bIsDashing;

		bWantsToTeleport = AssassinsCMC->bWantsToTeleport;
		if (bWantsToTeleport)
		{
			TeleportLocation = AssassinsCMC->TeleportLocation;
			TeleportRotation = AssassinsCMC->TeleportRotation;
		}
	}
}

bool FSavedMove_AssassinsCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_AssassinsCharacter* NewMoveCast = static_cast<FSavedMove_AssassinsCharacter*>(NewMove.Get());
	if (NewMoveCast == nullptr)
	{
		return false;
	}

	if (bIsDashing != NewMoveCast->bIsDashing)
	{
		return false;
	}

	if (bWantsToTeleport || NewMoveCast->bWantsToTeleport)
	{
		return false;
	}

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_AssassinsCharacter::PrepMoveFor(ACharacter* C)
{
	UAssassinsCharacterMovementComponent* AssassinsCMC = Cast<UAssassinsCharacterMovementComponent>(C->GetCharacterMovement());

	// A move made without root motion replays without it. The engine puts back only the root motion a move had, and
	// otherwise leaves what the group holds: the root motion of the move replayed before(a dash that ended in between,
	// its source finished or taken away as the dash stopped), which would carry the dash on, or for the first move
	// replayed, the root motion going on now(a dash that started after this move).
	if (AssassinsCMC && !SavedRootMotion.HasActiveRootMotionSources() && AssassinsCMC->CurrentRootMotion.HasActiveRootMotionSources())
	{
		FRootMotionSourceGroup& RootMotion = AssassinsCMC->CurrentRootMotion;

		// The sources of a move replayed before end as they did then, with their finish settings(the clamp of the velocity,
		// for a dash): they are the replay's own copies. Those going on now are not touched, but only let go: the replay
		// shares them with what it puts back once it is over(UCharacterMovementComponent::ClientUpdatePositionAfterServerUpdate).
		const FNetworkPredictionData_Client_Character* ClientData = AssassinsCMC->GetPredictionData_Client_Character();
		if (ClientData && !ClientData->SavedMoves.IsEmpty() && (ClientData->SavedMoves[0].Get() != this))
		{
			for (const TSharedPtr<FRootMotionSource>& RootMotionSource : RootMotion.RootMotionSources)
			{
				if (RootMotionSource.IsValid())
				{
					RootMotionSource->Status.SetFlag(ERootMotionSourceStatusFlags::MarkedForRemoval);
				}
			}
			RootMotion.CleanUpInvalidRootMotion(DeltaTime, *C, *AssassinsCMC);
		}
		RootMotion.Clear();
	}

	Super::PrepMoveFor(C);

	if (AssassinsCMC)
	{
		AssassinsCMC->bIsDashing = bIsDashing;

		AssassinsCMC->bWantsToTeleport = bWantsToTeleport;
		if (AssassinsCMC->bWantsToTeleport)
		{
			AssassinsCMC->TeleportLocation = TeleportLocation;
			AssassinsCMC->TeleportRotation = TeleportRotation;
		}
	}
}

FNetworkPredictionData_Client_AssassinsCharacter::FNetworkPredictionData_Client_AssassinsCharacter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_AssassinsCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_AssassinsCharacter());
}

/////////////////////////////////////////////////////////////////////////
/// Data transferred between server and client
/////////////////////////////////////////////////////////////////////////

FAssassinsCharacterNetworkMoveData::FAssassinsCharacterNetworkMoveData()
{
	bWantsToTeleport = 0;
	TeleportLocation = FVector::ZeroVector;
	TeleportRotation = FRotator::ZeroRotator;
}

void FAssassinsCharacterNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);

	const FSavedMove_AssassinsCharacter& SavedMove = static_cast<const FSavedMove_AssassinsCharacter&>(ClientMove);

	bWantsToTeleport = SavedMove.bWantsToTeleport;
	if (bWantsToTeleport)
	{
		TeleportLocation = SavedMove.TeleportLocation;
		TeleportRotation = SavedMove.TeleportRotation;
	}
}

bool FAssassinsCharacterNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	if (Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType))
	{
		bool bLocalSuccess = true;

		Ar << bWantsToTeleport;
		if (bWantsToTeleport)
		{
			TeleportLocation.NetSerialize(Ar, PackageMap, bLocalSuccess);
			TeleportRotation.NetSerialize(Ar, PackageMap, bLocalSuccess);
		}

		return !Ar.IsError();
	}

	return false;
}

FAssassinsCharacterNetworkMoveDataContainer::FAssassinsCharacterNetworkMoveDataContainer()
{
	NewMoveData = &AssassinsMoveData[0];
	PendingMoveData = &AssassinsMoveData[1];
	OldMoveData = &AssassinsMoveData[2];
}

FAssassinsCharacterMoveResponseDataContainer::FAssassinsCharacterMoveResponseDataContainer()
{
}

void FAssassinsCharacterMoveResponseDataContainer::ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment)
{
	Super::ServerFillResponseData(CharacterMovement, PendingAdjustment);
}

bool FAssassinsCharacterMoveResponseDataContainer::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap)
{
	return Super::Serialize(CharacterMovement, Ar, PackageMap);
}
