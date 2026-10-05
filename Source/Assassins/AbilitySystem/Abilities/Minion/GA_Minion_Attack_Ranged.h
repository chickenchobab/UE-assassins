#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Ranged.h"
#include "GA_Minion_Attack_Ranged.generated.h"

UCLASS(Abstract)
class UGA_Minion_Attack_Ranged : public UGA_Attack_Ranged
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
