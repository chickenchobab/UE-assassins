#pragma once

#include "AbilitySystem/Abilities/Attack/Ability_RangedAttack.h"
#include "Ability_Minion_RangedAttack.generated.h"

UCLASS(Abstract)
class UAbility_Minion_RangedAttack : public UAbility_RangedAttack
{
	GENERATED_BODY()

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void SetMontageToPlay() override;

	virtual void SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform) override;
	virtual void HandleProjectile_Implementation(AAssassinsProjectile* SpawnedProjectile) override;

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	FLinearColor GetTeamColorFromTeamInfo() const;
};
