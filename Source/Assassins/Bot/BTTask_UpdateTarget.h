#pragma once

#include "BehaviorTree/BTTaskNode.h"

#include "BTTask_UpdateTarget.generated.h"

UCLASS(Meta = (DisplayName = "Update Target"))
class UBTTask_UpdateTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBTTask_UpdateTarget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector FoundTargetKey;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector TargetKey;
};
