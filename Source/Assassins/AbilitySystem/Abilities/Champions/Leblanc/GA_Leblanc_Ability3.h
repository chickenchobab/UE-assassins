// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GA_Leblanc_Ability3.generated.h"

class AAssassinsProjectile;
class UGameplayEffect;

/**
 * UGA_Leblanc_Ability3
 *
 * Leblanc E, Ethereal Chains. Leblanc throws a chain; the unit it hits is tethered to her, and held in place when the
 * tether lasts, unless it walks out of range first. The server decides when it is over: the chain may still fly, or
 * the tether still hold, when the montage ends. Was the GA_Leblanc_Ability3 blueprint.
 */
UCLASS(Abstract)
class UGA_Leblanc_Ability3 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Ability3(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_LocationTargeted_Immediate interface
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override;
	virtual void OnMontageCancelled() override;
	//~End of UGA_LocationTargeted_Immediate interface

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileRange = 0.0;

	// Carried by the chain to what it hits. It grants the tether tag.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// Beyond this distance the tether breaks.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double TetherRange = 0.0;

	// Played when Leblanc stands still, or runs, as she throws.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> IdleMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> RunMontage;

	// Where on the mesh the chain leaves from.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName ProjectileSpawnSocket;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AActor> ChainTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle DamageEffectSpecHandle;

	// The tether tag the damage effect grants, watched on the target.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayTag TetherEffectTag;

private:

	// Picks the montage by whether Leblanc moves. She keeps running while she throws.
	void AdjustToMovement();

	void SpawnProjectile();
	void HandleProjectile(AAssassinsProjectile* SpawnedProjectile);
	void HandleChainHit();
	void EndIfNothingPending();
	void CheckChainMissed();
	void BreakTether();

	// Out of TetherRange, or gone.
	bool IsTargetOutOfTetherRange() const;

	UFUNCTION()
	void OnSpawnProjectileNotify(FGameplayEventData Payload);

	UFUNCTION()
	void OnSpawnProjectileRequested();

	UFUNCTION()
	void OnChainHit(FGameplayEventData Payload);

	UFUNCTION()
	void OnTetherTick(float DeltaTime);

	UFUNCTION()
	void OnTetherRemoved();

	UFUNCTION()
	void OnProjectileDestroyed(AActor* DestroyedActor);

	UPROPERTY()
	TObjectPtr<AAssassinsProjectile> ThrownProjectile;

	bool bMontageOver = false;
	bool bChainFlying = false;
	bool bChainHit = false;
};
