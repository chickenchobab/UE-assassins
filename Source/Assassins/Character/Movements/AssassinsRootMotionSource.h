#pragma once

#include "GameFramework/RootMotionSource.h"

#include "AssassinsRootMotionSource.generated.h"

USTRUCT()
struct FRootMotionSource_MoveToDynamicConstantSpeed : public FRootMotionSource
{
	GENERATED_BODY();

	FRootMotionSource_MoveToDynamicConstantSpeed();

	UPROPERTY()
	FVector StartLocation;

	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	float Speed;

	ASSASSINS_API void SetTargetLocation(FVector NewTargetLocation);

	//~FRootMotionSource interface
	ASSASSINS_API virtual void PrepareRootMotion(
		float SimulationTime,
		float MovementTickTime,
		const ACharacter& Character,
		const UCharacterMovementComponent& MoveComponent
	) override;
	ASSASSINS_API virtual FRootMotionSource* Clone() const override;
	ASSASSINS_API virtual FString ToSimpleString() const override;
	//~End of FRootMotionSource interface
};