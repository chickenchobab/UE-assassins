#include "Bot/EnvQueryTest_CurrentTarget.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"

UEnvQueryTest_CurrentTarget::UEnvQueryTest_CurrentTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_ActorBase::StaticClass();
	TestPurpose = EEnvTestPurpose::Score;

	// EQS_ScoreAttackTarget의 우선순위 단계 점수는 4, 8, 16, ...이고 거리 점수는 1 이하다. 1.5면 같은 단계에서는 지금 대상이
	// 남고(거리 차이 1 < 1.5), 더 높은 단계에는 늘 밀린다(가장 작은 단계 차이 4 > 1.5 + 1).
	ScoringFactor.DefaultValue = 1.5f;

	SetWorkOnFloatValues(false);
}

void UEnvQueryTest_CurrentTarget::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	AActor* Querier = Cast<AActor>(QueryOwner);
	if (Querier == nullptr)
	{
		return;
	}

	BoolValue.BindData(QueryOwner, QueryInstance.QueryID);
	const bool bWantsCurrent = BoolValue.GetValue();

	const UBlackboardComponent* Blackboard = UAIBlueprintHelperLibrary::GetBlackboard(Querier);
	const UObject* CurrentTarget = Blackboard ? Blackboard->GetValueAsObject(TargetKeyName) : nullptr;

	FEnvQueryInstance::ItemIterator It(this, QueryInstance);
	for (It.IgnoreTimeLimit(); It; ++It)
	{
		const bool bIsCurrent = (CurrentTarget != nullptr) && (GetItemActor(QueryInstance, It.GetIndex()) == CurrentTarget);
		It.SetScore(TestPurpose, FilterType, bIsCurrent, bWantsCurrent);
	}
}

FText UEnvQueryTest_CurrentTarget::GetDescriptionDetails() const
{
	return DescribeBoolTestParams(FString::Printf(TEXT("blackboard %s"), *TargetKeyName.ToString()));
}
