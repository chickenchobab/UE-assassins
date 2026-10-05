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
	// TeleportCharacter로 지난 이동 뒤 요청한 순간이동은 아직 저장된 이동에 없다. 서버 보정 뒤 저장된 이동을 다시 돌리면
	// 이동마다 플래그를 다시 설정해(PrepMoveFor) 요청이 사라지고, 순간이동이 여기서도, 이동에서 받는 서버에서도 일어나지 않는다.
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

	// 돌진 모드는 돌진 루트 모션이 있는 동안 이어진다. 루트 모션이 사라지면(대상에 닿는 이동에서 끝났거나 돌진이 멈춤) 그 이동부터 걷는다.
	// 이동을 시뮬레이션하는 모든 쪽에서 같다. 서버의 모드를 가져오는 보정이 돌진이 끝난 클라이언트에 이 모드를 남길 수도 있는데,
	// 이 모드에서는 루트 모션만 캐릭터를 움직인다.
	if (!CurrentRootMotion.HasOverrideVelocity())
	{
		// 캐릭터의 돌진 종료(월드 스태틱이 다시 막음)도 이 이동에서 한다. 돌진 태스크가 끝날 때로 미루면 서버는 클라이언트 이동이 더 온 뒤에야 본다.
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

	// 돌진이 이 이동에서 대상에 닿았다. 붙들던 것은 바로 시작할 수 있다. 서버에서는 착지 순간 누른 어빌리티가
	// 돌진 태스크의 다음 틱 전에 이 이동과 함께 올 수 있다(Migration/ISSUES.md 103). 태스크는 자기 틱에서 어빌리티에 돌진이 끝났다고 알린다.
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

	// 루트 모션 없이 한 이동은 루트 모션 없이 다시 돌린다. 엔진은 이동이 가졌던 루트 모션만 되돌리고 나머지는 그룹에 남겨 두는데,
	// 그러면 앞서 다시 돌린 이동의 루트 모션(사이에 끝난 돌진)이 돌진을 이어 가거나, 첫 이동이면 지금 진행 중인 루트 모션(이 이동 뒤 시작한 돌진)이 적용된다.
	if (AssassinsCMC && !SavedRootMotion.HasActiveRootMotionSources() && AssassinsCMC->CurrentRootMotion.HasActiveRootMotionSources())
	{
		FRootMotionSourceGroup& RootMotion = AssassinsCMC->CurrentRootMotion;

		// 앞서 다시 돌린 이동의 소스는 그때처럼 끝 설정(돌진이면 속도 제한)으로 끝낸다. 리플레이 자신의 사본이다.
		// 지금 진행 중인 소스는 건드리지 않고 놓기만 한다. 리플레이가 끝나면 되돌릴 것과 공유하기 때문이다(UCharacterMovementComponent::ClientUpdatePositionAfterServerUpdate).
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
