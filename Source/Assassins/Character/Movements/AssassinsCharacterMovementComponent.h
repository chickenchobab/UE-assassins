// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "AssassinsCharacterMovementComponent.generated.h"

USTRUCT()
struct FMoveRequestForReachTest
{
	GENERATED_BODY()

	FMoveRequestForReachTest();

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	void Init(float TimeStamp);
	bool IsValid() { return bIsValid; }

	UPROPERTY()
	bool bIsValid;

	UPROPERTY()
	float RequestTimeStamp;

	UPROPERTY()
	bool bMoveToActor;

	UPROPERTY()
	AActor* GoalActor;

	UPROPERTY()
	FVector GoalLocation;

	UPROPERTY()
	float AcceptanceRadius;

	UPROPERTY(Transient)
	bool bReachTestIncludesAgentRadius;

	UPROPERTY(Transient)
	bool bReachTestIncludesGoalRadius;
};

template<>
struct TStructOpsTypeTraits<FMoveRequestForReachTest> : public TStructOpsTypeTraitsBase2<FMoveRequestForReachTest>
{
	enum
	{
		WithNetSerializer = true,
		WithNetSharedSerialization = false // Because the struct is for server RPC.
	};
};

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

	FMoveRequestForReachTest MoveRequest;
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
public:

	FAssassinsCharacterNetworkMoveData();

	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;

	FMoveRequestForReachTest MoveRequest;
};

struct FAssassinsCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
public:

	FAssassinsCharacterNetworkMoveDataContainer();

private:

	FAssassinsCharacterNetworkMoveData AssassinsMoveData[3];
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
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	//~End of UCharacterMovementComponent interface

	// Path following component in the client calls the function
	void SetMoveRequest(const FMoveRequestForReachTest& ReachTestRequest);

public:

	void PhysDashing(float deltaTime, int32 Iterations);

	uint8 bIsDashing : 1;

	FMoveRequestForReachTest MoveRequest;

private:

	float TimeStampForMoveRequestDone;

	FAssassinsCharacterNetworkMoveDataContainer AssassinsNetworkMoveDataContainer;
};