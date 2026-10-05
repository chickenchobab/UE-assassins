#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Ranged.h"
#include "GA_Leblanc_Attack.generated.h"

UCLASS(Abstract)
class UGA_Leblanc_Attack : public UGA_Attack_Ranged
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform) override;
};
