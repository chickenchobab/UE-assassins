#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "BTTask_CancelAttackAndClearTarget.generated.h"

UCLASS(Meta = (DisplayName = "Cancel Attack And Clear Target"))
class UBTTask_CancelAttackAndClearTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_CancelAttackAndClearTarget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector TargetKey;
};
