// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Net/UnrealNetwork.h"
#include "AssassinsLogCategories.h"

UAssassinsCharacterMovementComponent::UAssassinsCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsDashing = 0;
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

	// Me: The capsule collision to WorldStatic must have been disabled but blocking should be checked on completion (TODO)

	// Me: Use specified speed instead of the power set by the root motion source
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
}

void FSavedMove_AssassinsCharacter::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (const UAssassinsCharacterMovementComponent* AssassinsCharacterMovement = Cast<UAssassinsCharacterMovementComponent>(C->GetCharacterMovement()))
	{
		bIsDashing = AssassinsCharacterMovement->bIsDashing;
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
