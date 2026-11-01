#pragma once

#include "Bot/BTDecorator_TickingCondition.h"
#include "GameplayTagContainer.h"

#include "BTDecorator_HasAnyStatus.generated.h"

UCLASS(Meta = (DisplayName = "Has Any Status"))
class UBTDecorator_HasAnyStatus : public UBTDecorator_TickingCondition
{
	GENERATED_BODY()

public:

	UBTDecorator_HasAnyStatus(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual FString GetStaticDescription() const override;

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	UPROPERTY(EditAnywhere, Category = Condition)
	FGameplayTagContainer StatusTags;
};
