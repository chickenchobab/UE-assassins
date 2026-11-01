#include "Bot/BTDecorator_TickingCondition.h"

#include "BehaviorTree/BehaviorTreeComponent.h"

UBTDecorator_TickingCondition::UBTDecorator_TickingCondition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bNotifyTick = true;
}

void UBTDecorator_TickingCondition::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// 조건이 블랙보드 키가 아니라 태그·거리라서 바뀌는 순간을 알려 주는 곳이 없다. 그래서 UBTDecorator_BlueprintBase처럼
	// 관찰 중인 동안(중단 모드가 있을 때만 관찰된다) 매 틱 다시 판정해, 실행 중인 가지는 거짓이 되면 멈추고
	// 실행 중이 아닌 가지는 참이 되면 끼어든다.
	if (OwnerComp.IsExecutingBranch(GetMyNode(), GetChildIndex()))
	{
		if (((FlowAbortMode == EBTFlowAbortMode::Self) || (FlowAbortMode == EBTFlowAbortMode::Both)) &&
			(CalculateRawConditionValue(OwnerComp, NodeMemory) == IsInversed()))
		{
			OwnerComp.RequestBranchDeactivation(*this);
		}
	}
	else if (((FlowAbortMode == EBTFlowAbortMode::LowerPriority) || (FlowAbortMode == EBTFlowAbortMode::Both)) &&
		(CalculateRawConditionValue(OwnerComp, NodeMemory) != IsInversed()))
	{
		OwnerComp.RequestBranchActivation(*this, false);
	}
}
