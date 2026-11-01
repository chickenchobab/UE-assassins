#pragma once

#include "EnvironmentQuery/EnvQueryTest.h"

#include "EnvQueryTest_CurrentTarget.generated.h"

UCLASS()
class UEnvQueryTest_CurrentTarget : public UEnvQueryTest
{
	GENERATED_BODY()

public:

	UEnvQueryTest_CurrentTarget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionDetails() const override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = Blackboard)
	FName TargetKeyName = TEXT("TargetEnemy");
};
