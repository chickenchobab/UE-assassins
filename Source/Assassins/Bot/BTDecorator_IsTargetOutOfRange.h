#pragma once

#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Bot/BTDecorator_TickingCondition.h"

#include "BTDecorator_IsTargetOutOfRange.generated.h"

UCLASS(Meta = (DisplayName = "Is Target Out Of Range"))
class UBTDecorator_IsTargetOutOfRange : public UBTDecorator_TickingCondition
{
	GENERATED_BODY()

public:

	UBTDecorator_IsTargetOutOfRange(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector RangeKey;

	UPROPERTY(EditAnywhere, Category = Condition, Meta = (ClampMin = "0.0", Units = "cm"))
	float RangeMargin = 200.0f;
};
