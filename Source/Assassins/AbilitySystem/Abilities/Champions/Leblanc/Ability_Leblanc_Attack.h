#pragma once

#include "AbilitySystem/Abilities/Attack/Ability_RangedAttack.h"
#include "Ability_Leblanc_Attack.generated.h"

UCLASS(Abstract)
class UAbility_Leblanc_Attack : public UAbility_RangedAttack
{
	GENERATED_BODY()

public:

	UAbility_Leblanc_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform) override;
};
