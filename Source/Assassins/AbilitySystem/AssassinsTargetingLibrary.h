#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "AssassinsTargetingLibrary.generated.h"

UCLASS()
class UAssassinsTargetingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Assassins|Targeting")
	static double GetDistance2DToTargetEdge(const AActor* Target, const FVector& FromLocation);

	UFUNCTION(BlueprintPure, Category = "Assassins|Targeting")
	static bool IsAttackableTarget(const AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "Assassins|Targeting")
	static void MarkTargetedByMinion(AActor* Target);
};
