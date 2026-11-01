#pragma once

#include "GameFramework/Actor.h"
#include "AssassinsPredictedActor.generated.h"

UCLASS(Abstract)
class ASSASSINS_API AAssassinsPredictedActor : public AActor
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	bool ShouldSkipCosmetic() const;
};
