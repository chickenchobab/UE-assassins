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

	// 대상은 비교하지 않는다. 시간에 따라 바뀌고(액터로 가는 돌진은 그 액터를 쫓는데 쪽마다 위치가 다르다), 서버의 소스는 이 규칙으로 클라이언트의 소스와 짝지어진다.
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

	const FVector CurrentLocation = Character.GetActorLocation();
	const FVector ToTarget(TargetLocation.X - CurrentLocation.X, TargetLocation.Y - CurrentLocation.Y, 0.0);
	const double RemainingDistance = ToTarget.Size();
	const double StepDistance = Speed * SimulationTime;

	// AcceptRadius 안에 드는 이동은 대상까지 가서 소스를 끝낸다. 그래서 돌진의 끝이 이동 자체에 들어가, 이동을 시뮬레이션하는 모든 쪽
	// (소유 클라이언트, 서버, 클라이언트의 리플레이)에서 같다. 프레임마다 한 번 보는 어빌리티 태스크라면 프레임이 가져온 이동 수만큼 늦게 끝낸다.
	// 마지막 걸음은 대상을 넘지 않아 다음 이동에서 되돌아오지 않는다.
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
