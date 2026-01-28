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
};
