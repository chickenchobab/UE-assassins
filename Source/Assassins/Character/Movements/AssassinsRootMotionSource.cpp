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

FRootMotionSource* FRootMotionSource_MoveToDynamicConstantSpeed::Clone() const
{
	FRootMotionSource_MoveToDynamicConstantSpeed* CopyPtr = new FRootMotionSource_MoveToDynamicConstantSpeed(*this);
	return CopyPtr;
}

bool FRootMotionSource_MoveToDynamicConstantSpeed::Matches(const FRootMotionSource* Other) const
{
	if (!FRootMotionSource::Matches(Other))
	{
		return false;
	}

	const FRootMotionSource_MoveToDynamicConstantSpeed* OtherCast = static_cast<const FRootMotionSource_MoveToDynamicConstantSpeed*>(Other);

	return Speed == OtherCast->Speed && FVector::PointsAreNear(TargetLocation, OtherCast->TargetLocation, 0.1f);
}

bool FRootMotionSource_MoveToDynamicConstantSpeed::MatchesAndHasSameState(const FRootMotionSource* Other) const
{
	return FRootMotionSource::MatchesAndHasSameState(Other);
}

bool FRootMotionSource_MoveToDynamicConstantSpeed::UpdateStateFrom(const FRootMotionSource* SourceToTakeStateFrom, bool bMarkForSimulatedCatchup)
{
	return FRootMotionSource::UpdateStateFrom(SourceToTakeStateFrom, bMarkForSimulatedCatchup);
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

bool FRootMotionSource_MoveToDynamicConstantSpeed::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	if (!FRootMotionSource::NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}

	Ar << StartLocation;
	Ar << TargetLocation;
	Ar << Speed;

	bOutSuccess = true;
	return true;
}

UScriptStruct* FRootMotionSource_MoveToDynamicConstantSpeed::GetScriptStruct() const
{
	return FRootMotionSource_MoveToDynamicConstantSpeed::StaticStruct();
}

FString FRootMotionSource_MoveToDynamicConstantSpeed::ToSimpleString() const
{
	return FString::Printf(TEXT("[ID:%u]FRootMotionSource_MoveToDynamicConstantSpeed %s"), LocalID, *InstanceName.GetPlainNameString());
}

void FRootMotionSource_MoveToDynamicConstantSpeed::AddReferencedObjects(FReferenceCollector& Collector)
{
	FRootMotionSource::AddReferencedObjects(Collector);
}
