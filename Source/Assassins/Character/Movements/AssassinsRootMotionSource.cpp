#include "AssassinsRootMotionSource.h"
#include "GameFramework/Character.h"

FRootMotionSource_MoveToDynamicConstantSpeed::FRootMotionSource_MoveToDynamicConstantSpeed()
	: StartLocation(ForceInitToZero)
	, TargetLocation(ForceInitToZero)
	, Speed(0.f)
	, AcceptRadius(0.f)
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

	// Not the target: it changes over time(it follows the actor of a dash to an actor, which each side sees at its own
	// place), and the server's sources are paired with the client's by this rule for as long as they last.
	return Speed == OtherCast->Speed;
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

	// On the XY plane.
	const FVector CurrentLocation = Character.GetActorLocation();
	const FVector ToTarget(TargetLocation.X - CurrentLocation.X, TargetLocation.Y - CurrentLocation.Y, 0.0);
	const double RemainingDistance = ToTarget.Size();
	const double StepDistance = Speed * SimulationTime;

	// The move that gets within AcceptRadius goes all the way to the target and ends the source. So the end of the dash is
	// part of the move itself: the same on every side that simulates the move(the owning client, the server, a replay of
	// the client), where an ability task looking once a frame would end it after however many moves the frame brought.
	// And the last step never goes past the target, to come back on the next.
	FVector Step = FVector::ZeroVector;
	if (SimulationTime > 0.f)
	{
		if (RemainingDistance <= StepDistance + AcceptRadius)
		{
			Step = ToTarget;
			Status.SetFlag(ERootMotionSourceStatusFlags::Finished);
		}
		else
		{
			Step = ToTarget * (StepDistance / RemainingDistance);
		}
	}

	// A velocity, which the movement applies for the whole move.
	const FVector Force = (MovementTickTime > UE_SMALL_NUMBER) ? (Step / MovementTickTime) : FVector::ZeroVector;
	RootMotionParams.Set(FTransform(Force));

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
	Ar << AcceptRadius;

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
