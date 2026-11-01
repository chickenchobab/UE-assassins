#pragma once

#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Bot/BTDecorator_TickingCondition.h"

#include "BTDecorator_FoundDifferentTarget.generated.h"

UCLASS(Meta = (DisplayName = "Found Different Target"))
class UBTDecorator_FoundDifferentTarget : public UBTDecorator_TickingCondition
{
	GENERATED_BODY()

public:

	UBTDecorator_FoundDifferentTarget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector FoundTargetKey;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector TargetKey;
};
