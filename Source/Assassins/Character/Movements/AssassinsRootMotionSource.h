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

	// How close to the target counts as there: the move that gets this close goes all the way to the target, and the
	// source ends with it.
	UPROPERTY()
	float AcceptRadius;

	ASSASSINS_API void SetTargetLocation(FVector NewTargetLocation);

	//~FRootMotionSource interface
	ASSASSINS_API virtual FRootMotionSource* Clone() const override;
	ASSASSINS_API virtual bool Matches(const FRootMotionSource* Other) const override;
	ASSASSINS_API virtual bool MatchesAndHasSameState(const FRootMotionSource* Other) const override;
	ASSASSINS_API virtual bool UpdateStateFrom(const FRootMotionSource* SourceToTakeStateFrom, bool bMarkForSimulatedCatchup = false) override;
	ASSASSINS_API virtual void PrepareRootMotion(
		float SimulationTime,
		float MovementTickTime,
		const ACharacter& Character,
		const UCharacterMovementComponent& MoveComponent
	) override;
	ASSASSINS_API virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	ASSASSINS_API virtual UScriptStruct* GetScriptStruct() const override;
	ASSASSINS_API virtual FString ToSimpleString() const override;
	virtual void AddReferencedObjects(class FReferenceCollector& Collector) override;
	//~End of FRootMotionSource interface
};

template<>
struct TStructOpsTypeTraits< FRootMotionSource_MoveToDynamicConstantSpeed > : public TStructOpsTypeTraitsBase2< FRootMotionSource_MoveToDynamicConstantSpeed >
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};