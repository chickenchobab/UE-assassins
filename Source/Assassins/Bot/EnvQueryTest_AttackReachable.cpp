#include "Bot/EnvQueryTest_AttackReachable.h"

#include "AI/Navigation/NavAgentInterface.h"
#include "Character/AssassinsCharacter.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"

namespace
{
	// 내비메시는 막힌 충돌에서 에이전트 반지름만큼 물러나 있어서, 그보다 조금 더 바깥을 본다.
	constexpr float StructureApproachMargin = 10.0f;

	ARecastNavMesh* FindNavMesh(UNavigationSystemV1& NavSys, UObject* Owner)
	{
		if (const INavAgentInterface* NavAgent = Cast<INavAgentInterface>(Owner))
		{
			return Cast<ARecastNavMesh>(NavSys.GetNavDataForProps(NavAgent->GetNavAgentPropertiesRef(), NavAgent->GetNavAgentLocation()));
		}

		return Cast<ARecastNavMesh>(NavSys.GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
	}

	// 구조물의 위치는 폰을 막는 충돌 한가운데라 내비메시로 닿지 않는다. 구조물은 질의자 쪽 가장자리 바깥, 공격하러 설 자리를 본다.
	// 구조물인지는 대상 추적, 사거리 판정과 같은 기준이다(UAssassinsTargetChasingComponent::ChaseTarget, UAbility_UnitTargeted::IsTargetInRange).
	FVector GetAttackLocation(const AActor& Target, const FVector& FromLocation, float AgentRadius)
	{
		const FVector TargetLocation = Target.GetActorLocation();
		if (Target.IsA<AAssassinsCharacter>())
		{
			return TargetLocation;
		}

		FVector Direction = (FromLocation - TargetLocation).GetSafeNormal2D();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector::ForwardVector;
		}

		return TargetLocation + Direction * (Target.GetSimpleCollisionRadius() + AgentRadius + StructureApproachMargin);
	}
}

UEnvQueryTest_AttackReachable::UEnvQueryTest_AttackReachable(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Context = UEnvQueryContext_Querier::StaticClass();
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_ActorBase::StaticClass();
	TestPurpose = EEnvTestPurpose::Filter;

	SetWorkOnFloatValues(false);
}

void UEnvQueryTest_AttackReachable::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(QueryInstance.World);
	if ((QueryOwner == nullptr) || (NavSys == nullptr))
	{
		return;
	}

	ARecastNavMesh* NavMesh = FindNavMesh(*NavSys, QueryOwner);
	TArray<FVector> ContextLocations;
	if ((NavMesh == nullptr) || !QueryInstance.PrepareContext(Context, ContextLocations))
	{
		return;
	}

	BoolValue.BindData(QueryOwner, QueryInstance.QueryID);
	const bool bWantsReachable = BoolValue.GetValue();

	const FSharedConstNavQueryFilter NavFilter = NavMesh->GetDefaultQueryFilter();
	const int32 NumItems = QueryInstance.Items.Num();

	// UEnvQueryTest_PathfindingBatch처럼 맥락 위치마다 갈 수 있는 폴리곤을 한 번 모으고, 각 항목의 공격할 자리가 그 안에 있는지 본다.
	// 공격할 자리가 맥락 위치에 따라 달라서 투영도 맥락 위치마다 한다. 모든 맥락 위치에서 닿아야 통과한다.
	TArray<bool> Reachable;
	Reachable.Init(true, NumItems);

	for (const FVector& ContextLocation : ContextLocations)
	{
		TArray<FNavigationProjectionWork> AttackPoints;
		TArray<int32> AttackPointItems;
		FVector::FReal MaxDistanceSq = 0.0;

		for (int32 ItemIndex = 0; ItemIndex < NumItems; ++ItemIndex)
		{
			if (!QueryInstance.Items[ItemIndex].IsValid())
			{
				continue;
			}

			const AActor* ItemActor = GetItemActor(QueryInstance, ItemIndex);
			if (ItemActor == nullptr)
			{
				Reachable[ItemIndex] = false;
				continue;
			}

			const FVector AttackLocation = GetAttackLocation(*ItemActor, ContextLocation, NavMesh->AgentRadius);
			AttackPoints.Emplace(AttackLocation);
			AttackPointItems.Add(ItemIndex);
			MaxDistanceSq = FMath::Max(MaxDistanceSq, FVector::DistSquared(AttackLocation, ContextLocation));
		}

		if (AttackPoints.IsEmpty())
		{
			continue;
		}

		NavMesh->BatchProjectPoints(AttackPoints, NavMesh->GetDefaultQueryExtent(), NavFilter, QueryOwner);

		TArray<NavNodeRef> Polys;
		NavMesh->GetPolysWithinPathingDistance(ContextLocation, FMath::Sqrt(MaxDistanceSq) * ScanRangeMultiplier, Polys, NavFilter, QueryOwner);
		TSet<NavNodeRef> ReachablePolys;
		ReachablePolys.Append(Polys);

		for (int32 PointIndex = 0; PointIndex < AttackPoints.Num(); ++PointIndex)
		{
			const FNavigationProjectionWork& AttackPoint = AttackPoints[PointIndex];
			if (!AttackPoint.bResult)
			{
				Reachable[AttackPointItems[PointIndex]] = false;
				continue;
			}

			// 모은 폴리곤은 맞닿은 변의 중점까지 거리를 이어 잰 것이라, 반지름 안이라도 변이 긴 이웃 폴리곤이 빠진다.
			// 그 반지름은 가장 먼 후보로 정해지므로, 집합에 없는 후보는 실제 경로로 다시 본다. 부분 경로는 실패로 친다.
			if (!ReachablePolys.Contains(AttackPoint.OutLocation.NodeRef))
			{
				const FPathFindingQuery PathQuery(QueryOwner, *NavMesh, ContextLocation, AttackPoint.OutLocation.Location, NavFilter);
				if (!NavSys->TestPathSync(PathQuery))
				{
					Reachable[AttackPointItems[PointIndex]] = false;
				}
			}
		}
	}

	FEnvQueryInstance::ItemIterator It(this, QueryInstance);
	for (It.IgnoreTimeLimit(); It; ++It)
	{
		It.SetScore(TestPurpose, FilterType, Reachable[It.GetIndex()], bWantsReachable);
	}
}

FText UEnvQueryTest_AttackReachable::GetDescriptionDetails() const
{
	return DescribeBoolTestParams(TEXT("reachable attack location"));
}
