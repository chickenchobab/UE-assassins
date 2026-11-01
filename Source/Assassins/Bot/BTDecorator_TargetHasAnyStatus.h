#pragma once

#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Bot/BTDecorator_TickingCondition.h"
#include "GameplayTagContainer.h"

#include "BTDecorator_TargetHasAnyStatus.generated.h"

UCLASS(Meta = (DisplayName = "Target Has Any Status"))
class UBTDecorator_TargetHasAnyStatus : public UBTDecorator_TickingCondition
{
	GENERATED_BODY()

public:

	UBTDecorator_TargetHasAnyStatus(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, Category = Condition)
	FGameplayTagContainer StatusTags;
};
