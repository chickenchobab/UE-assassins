#pragma once

#include "EnvironmentQuery/EnvQueryGenerator.h"

#include "EnvQueryGenerator_EnemiesInRange.generated.h"

UCLASS(Meta = (DisplayName = "Enemies In Range"))
class UEnvQueryGenerator_EnemiesInRange : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:

	UEnvQueryGenerator_EnemiesInRange(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = Generator)
	FName RangeKeyName = TEXT("AggroRange");

	UPROPERTY(EditDefaultsOnly, Category = Generator, Meta = (ClampMin = "0.0", Units = "cm"))
	float DefaultRange = 500.0f;
};
