#pragma once

#include "BehaviorTree/BTDecorator.h"

#include "BTDecorator_TickingCondition.generated.h"

UCLASS(Abstract)
class UBTDecorator_TickingCondition : public UBTDecorator
{
	GENERATED_BODY()

public:

	UBTDecorator_TickingCondition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
