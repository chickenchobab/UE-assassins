// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "AssassinsCharacterMovementComponent.generated.h"

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
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	//~End of UCharacterMovementComponent interface

	void PhysDashing(float deltaTime, int32 Iterations);

	uint8 bIsDashing : 1;
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
};

class FNetworkPredictionData_Client_AssassinsCharacter : public FNetworkPredictionData_Client_Character
{
public:

	FNetworkPredictionData_Client_AssassinsCharacter(const UCharacterMovementComponent& ClientMovement);

	typedef FNetworkPredictionData_Client_Character Super;

	virtual FSavedMovePtr AllocateNewMove() override;
};