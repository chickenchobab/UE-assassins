// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Player/AssassinsPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "AssassinsLogCategories.h"

FMoveRequestForReachTest::FMoveRequestForReachTest()
{
	bIsValid = false;
	RequestTimeStamp = 0.0f;

	bMoveToActor = false;
	GoalActor = nullptr;
}

bool FMoveRequestForReachTest::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << bIsValid;
	if (!bIsValid)
	{
		return true;
	}
	Ar << RequestTimeStamp;

	Ar << bMoveToActor;
	if (bMoveToActor)
	{
		if (GoalActor)
		{
			UObject* GoalActorObject = GoalActor;
			Map->SerializeObject(Ar, AActor::StaticClass(), GoalActorObject);
		}
	}
	else
	{
		Ar << GoalLocation;
	}

	Ar << AcceptanceRadius;
	Ar << bReachTestIncludesAgentRadius;
	Ar << bReachTestIncludesGoalRadius;

	return true;
}

void FMoveRequestForReachTest::Init(float TimeStamp)
{
	RequestTimeStamp = TimeStamp;

	bIsValid = true;
}

UAssassinsCharacterMovementComponent::UAssassinsCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsDashing = 0;

	TimeStampForMoveRequestDone = 0.0f;

	SetNetworkMoveDataContainer(AssassinsNetworkMoveDataContainer);
}

void UAssassinsCharacterMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel)
{
	AActor* MyOwner = GetOwner();
	check(MyOwner);

	NetworkMaxSmoothUpdateDistance = 512.0f;
	NetworkNoSmoothUpdateDistance = 640.0f;

	const bool bIsServerForAutonomousProxy = MyOwner->HasAuthority() && MyOwner->GetRemoteRole() == ROLE_AutonomousProxy;
	if (!bIsServerForAutonomousProxy)
	{
		Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
		return;
	}

	FAssassinsCharacterNetworkMoveData* MoveData = static_cast<FAssassinsCharacterNetworkMoveData*>(GetCurrentNetworkMoveData());
	FMoveRequestForReachTest ReachTestRequest = MoveData ? MoveData->MoveRequest : FMoveRequestForReachTest();
	if (!ReachTestRequest.IsValid())
	{
		Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
		return;
	}

	AAssassinsPlayerController* AssassinsPC = Cast<AAssassinsPlayerController>(GetController());
	check(AssassinsPC);

	if (ReachTestRequest.RequestTimeStamp <= TimeStampForMoveRequestDone || AssassinsPC->HasMoveReached(ReachTestRequest))
	{
		TimeStampForMoveRequestDone = FMath::Max(TimeStampForMoveRequestDone, ReachTestRequest.RequestTimeStamp);

		AssassinsPC->NotifyMoveSuccess();
		UE_LOG(LogAssassins, Display, TEXT("Request at [%f] has been already done so zeros the velocity and the acceleration"), TimeStampForMoveRequestDone);

		NetworkMaxSmoothUpdateDistance = 256.0f;
		NetworkNoSmoothUpdateDistance = 384.0f;

		Velocity = FVector::ZeroVector;
		Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, FVector::ZeroVector);

		return;
	}

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

FNetworkPredictionData_Client* UAssassinsCharacterMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UAssassinsCharacterMovementComponent* MutableThis = const_cast<UAssassinsCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_AssassinsCharacter(*this);
	}

	return ClientPredictionData;
}

void UAssassinsCharacterMovementComponent::SetMoveRequest(const FMoveRequestForReachTest& ReachTestRequest)
{
	MoveRequest = ReachTestRequest;
}

void UAssassinsCharacterMovementComponent::PhysDashing(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
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
}

FSavedMove_AssassinsCharacter::FSavedMove_AssassinsCharacter()
{
	bIsDashing = 0;
}

void FSavedMove_AssassinsCharacter::Clear()
{
	Super::Clear();

	bIsDashing = 0;

	MoveRequest = FMoveRequestForReachTest();
}

void FSavedMove_AssassinsCharacter::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const UAssassinsCharacterMovementComponent* AssassinsCharacterMovement = Cast<UAssassinsCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		bIsDashing = AssassinsCharacterMovement->bIsDashing;

		MoveRequest = AssassinsCharacterMovement->MoveRequest;
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

	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_AssassinsCharacter::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	
	if (UAssassinsCharacterMovementComponent* AssassinsCharacterMovement = Cast<UAssassinsCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		AssassinsCharacterMovement->bIsDashing = bIsDashing;

		AssassinsCharacterMovement->MoveRequest = MoveRequest;
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

FAssassinsCharacterNetworkMoveData::FAssassinsCharacterNetworkMoveData()
{
}

void FAssassinsCharacterNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	FCharacterNetworkMoveData::ClientFillNetworkMoveData(ClientMove, MoveType);

	const FSavedMove_AssassinsCharacter& SavedMove = static_cast<const FSavedMove_AssassinsCharacter&>(ClientMove);

	MoveRequest = SavedMove.MoveRequest;
}

bool FAssassinsCharacterNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	bool bLocalSuccess = true;

	if (FCharacterNetworkMoveData::Serialize(CharacterMovement, Ar, PackageMap, MoveType)) // 244
	{
		MoveRequest.NetSerialize(Ar, PackageMap, bLocalSuccess);
	}

	return !Ar.IsError();
}

FAssassinsCharacterNetworkMoveDataContainer::FAssassinsCharacterNetworkMoveDataContainer()
{
	NewMoveData = &AssassinsMoveData[0];
	PendingMoveData = &AssassinsMoveData[1];
	OldMoveData = &AssassinsMoveData[2];
}
