#include "AssassinsRootMotionSource.h"
#include "GameFramework/Character.h"

FRootMotionSource_MoveToDynamicConstantSpeed::FRootMotionSource_MoveToDynamicConstantSpeed()
	: StartLocation(ForceInitToZero)
	, TargetLocation(ForceInitToZero)
{
}

void FRootMotionSource_MoveToDynamicConstantSpeed::SetTargetLocation(FVector NewTargetLocation)
{
	TargetLocation = NewTargetLocation;
}

void FRootMotionSource_MoveToDynamicConstantSpeed::PrepareRootMotion
(
	float SimulationTime,
	float MovementTickTime,
	const ACharacter& Character,
	const UCharacterMovementComponent& MoveComponent
)
{
	RootMotionParams.Clear();

	const FVector CurrentLocation = Character.GetActorLocation();
	float Multiplier = (MovementTickTime > UE_SMALL_NUMBER) ? (SimulationTime / MovementTickTime) : 1.f;

	FVector Force = (TargetLocation - CurrentLocation).GetSafeNormal2D() * Speed * Multiplier;
	FTransform NewTransform(Force);

	RootMotionParams.Set(NewTransform);

	SetTime(GetTime() + SimulationTime);
}

FRootMotionSource* FRootMotionSource_MoveToDynamicConstantSpeed::Clone() const
{
	FRootMotionSource_MoveToDynamicConstantSpeed* CopyPtr = new FRootMotionSource_MoveToDynamicConstantSpeed(*this);
	return CopyPtr;
}

FString FRootMotionSource_MoveToDynamicConstantSpeed::ToSimpleString() const
{
	return FString::Printf(TEXT("[ID:%u]FRootMotionSource_MoveToDynamicConstantSpeed %s"), LocalID, *InstanceName.GetPlainNameString());
}