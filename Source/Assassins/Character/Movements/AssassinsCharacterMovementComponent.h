// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "AssassinsCharacterMovementComponent.generated.h"

class FSavedMove_AssassinsCharacter : public FSavedMove_Character
{
public:

	FSavedMove_AssassinsCharacter();

	typedef FSavedMove_Character Super;

	//~FSavedMove_Character interface
	virtual void Clear() override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void PrepMoveFor(ACharacter* C) override;
	//~End of FSavedMove_Character interface

	uint8 bIsDashing : 1;

	uint8 bWantsToTeleport : 1;
	FVector TeleportLocation;
	FRotator TeleportRotation;
};

class FNetworkPredictionData_Client_AssassinsCharacter : public FNetworkPredictionData_Client_Character
{
public:

	FNetworkPredictionData_Client_AssassinsCharacter(const UCharacterMovementComponent& ClientMovement);

	typedef FNetworkPredictionData_Client_Character Super;

	virtual FSavedMovePtr AllocateNewMove() override;
};

struct FAssassinsCharacterNetworkMoveData : public FCharacterNetworkMoveData
{
	typedef FCharacterNetworkMoveData Super;

public:
	FAssassinsCharacterNetworkMoveData();

	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;

	bool bWantsToTeleport;
	FVector TeleportLocation;
	FRotator TeleportRotation;
};

struct FAssassinsCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
public:

	FAssassinsCharacterNetworkMoveDataContainer();

private:

	FAssassinsCharacterNetworkMoveData AssassinsMoveData[3];
};

struct FAssassinsCharacterMoveResponseDataContainer : public FCharacterMoveResponseDataContainer
{
	typedef FCharacterMoveResponseDataContainer Super;

public:
	FAssassinsCharacterMoveResponseDataContainer();

	virtual void ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap) override;
};

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None		UMETA(Hidden),
	CMOVE_Dashing	UMETA(DisplayName = "Dashing")
};

/**
 * The base character movement component class used by this project.
 */
UCLASS(Config = Game)
class ASSASSINS_API UAssassinsCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:

	UAssassinsCharacterMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UCharacterMovementComponent interface
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void ServerMove_HandleMoveData(const FCharacterNetworkMoveDataContainer& MoveDataContainer) override;
	virtual void ServerMove_PerformMovement(const FCharacterNetworkMoveData& MoveData) override;
	virtual void ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse) override;
	virtual void ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, TOptional<FRotator> OptionalRotation = TOptional<FRotator>()) override;
	//~End of UCharacterMovementComponent interface

public:

	void PhysDashing(float deltaTime, int32 Iterations);

	void TeleportCharacter(FVector GoalLocation, FRotator GoalRotation);

	uint8 bIsDashing : 1;

	uint8 bWantsToTeleport : 1;
	FVector TeleportLocation;
	FRotator TeleportRotation;

private:
	FAssassinsCharacterNetworkMoveDataContainer AssassinsNetworkMoveDataContainer;
	FAssassinsCharacterMoveResponseDataContainer AssassinsMoveResponseDataContainer;
};