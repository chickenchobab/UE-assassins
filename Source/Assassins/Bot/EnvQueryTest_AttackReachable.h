#pragma once

#include "EnvironmentQuery/EnvQueryTest.h"

#include "EnvQueryTest_AttackReachable.generated.h"

UCLASS()
class UEnvQueryTest_AttackReachable : public UEnvQueryTest
{
	GENERATED_BODY()

public:

	UEnvQueryTest_AttackReachable(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionDetails() const override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = Pathfinding)
	TSubclassOf<UEnvQueryContext> Context;

	UPROPERTY(EditDefaultsOnly, Category = Pathfinding, AdvancedDisplay)
	float ScanRangeMultiplier = 1.5f;
};
