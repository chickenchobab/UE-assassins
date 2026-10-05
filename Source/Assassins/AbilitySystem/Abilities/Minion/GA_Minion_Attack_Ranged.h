// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Attack/GA_Attack_Ranged.h"
#include "GA_Minion_Attack_Ranged.generated.h"

/**
 * UGA_Minion_Attack_Ranged
 *
 * Ranged minion basic attack. The range and the montages come from the minion, and the projectile is dressed in
 * the team color before it leaves. Was the GA_Minion_Attack_Ranged blueprint.
 */
UCLASS(Abstract)
class UGA_Minion_Attack_Ranged : public UGA_Attack_Ranged
{
	GENERATED_BODY()

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~End of UGameplayAbility interface

	//~UGA_Attack interface
	virtual void SetMontageToPlay() override;
	//~End of UGA_Attack interface

	//~IAssassinsProjectileHandler interface
	virtual void SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform) override;
	virtual void HandleProjectile_Implementation(AAssassinsProjectile* SpawnedProjectile) override;
	//~End of IAssassinsProjectileHandler interface

	// The color of the team the minion belongs to, taken from the team asset.
	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	FLinearColor GetTeamColorFromTeamInfo() const;
};
