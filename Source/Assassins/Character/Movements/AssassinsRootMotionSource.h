#pragma once

#include "GameFramework/RootMotionSource.h"

#include "AssassinsRootMotionSource.generated.h"

USTRUCT()
struct FRootMotionSource_MoveToDynamicConstantSpeed : public FRootMotionSource
{
	GENERATED_BODY();

public:

	ASSASSINS_API FRootMotionSource_MoveToDynamicConstantSpeed();

	virtual ~FRootMotionSource_MoveToDynamicConstantSpeed() {}

	UPROPERTY()
	FVector StartLocation;

	UPROPERTY()
	FVector InitialTargetLocation;

	// Dynamically-changing location of target, which may be altered while this movement is ongoing 
	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	float Speed;

	ASSASSINS_API void SetTargetLocation(FVector NewTargetLocation);

	// Skip the functions for the replication for now.
	
	//~FRootMotionSource interface
	ASSASSINS_API virtual void PrepareRootMotion(
		float SimulationTime,
		float MovementTickTime,
		const ACharacter& Character,
		const UCharacterMovementComponent& MoveComponent
	) override;
	ASSASSINS_API virtual FString ToSimpleString() const override;
	//~End of FRootMotionSource interface
};

USTRUCT()
struct FRootMotionSource_MoveToConstantSpeed : public FRootMotionSource
{
	GENERATED_BODY();

public:

	ASSASSINS_API FRootMotionSource_MoveToConstantSpeed();

	virtual ~FRootMotionSource_MoveToConstantSpeed() {}

	UPROPERTY()
	FVector StartLocation;

	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	float Speed;

	// Skip the functions for the replication for now.

	//~FRootMotionSource interface
	ASSASSINS_API virtual void PrepareRootMotion(
		float SimulationTime,
		float MovementTickTime,
		const ACharacter& Character,
		const UCharacterMovementComponent& MoveComponent
	)override;
	ASSASSINS_API virtual FString ToSimpleString() const override;
	//~End of FRootMotionSource interface
};