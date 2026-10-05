// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GA_Akali_Ability3.generated.h"

class AAssassinsProjectile;
class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayTagRemoved;
class UCurveTable;
class UGameplayEffect;

/**
 * UGA_Akali_Ability3
 *
 * Akali E, Shuriken Flip. Akali flips back and throws a shuriken forward. What it hits is marked: pressing the
 * ability again dashes to the marked unit, or to the mark the shuriken left on her shroud. The ability lasts until
 * the dash is over, or until the mark is gone. After the dash to a unit only the server ends it, once its own dash
 * dealt the damage, and the owning client goes on attacking the unit as that end reaches it. Was the GA_Akali_Ability3
 * blueprint.
 */
UCLASS(Abstract)
class UGA_Akali_Ability3 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability3(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_LocationTargeted_Immediate interface
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}
	//~End of UGA_LocationTargeted_Immediate interface

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double FlipSpeed = 0.0;

	// How far back Akali flips.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double FlipDistance = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double DashSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	// Carried by the shuriken to what it hits. It also marks the target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> ProjectileDamageEffectClass;

	// Applied when the dash reaches the marked target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DashDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> DashMontage;

	// How long the mark left on the shroud lasts, by ability level.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UCurveTable> AbilityMagnitudeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName DashMarkDurationRow;

	// The unit the shuriken marked.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AActor> ProjectileHitTarget;

	// The mark the shuriken left on the shroud, where Akali dashes when no unit was hit.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector DashLocationWithoutTarget = FVector::ZeroVector;

private:

	void SpawnProjectile();
	void HandleProjectile(AAssassinsProjectile* SpawnedProjectile);
	void WaitForRecast();
	void StartDash();
	void EndDashMarkCheckingTask();
	void CheckProjectileMissed();
	void ApplyDashDamage();

	// Starts the basic attack at the target(Event.Attack), where the attack can start: on the side that controls Akali.
	void SendAttackEvent(AActor* AttackTarget);
	FVector CalculateFlipLocation() const;

	UFUNCTION()
	void OnFlipFinished();

	UFUNCTION()
	void OnShurikenHit(FGameplayEventData Payload);

	UFUNCTION()
	void OnDashMarkExpired();

	UFUNCTION()
	void OnTargetMarkRemoved();

	UFUNCTION()
	void OnProjectileDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnRecastReleased(float TimeHeld);

	UFUNCTION()
	void OnRecastPressed();

	UFUNCTION()
	void OnDashCancelled();

	UFUNCTION()
	void OnTargetDashFinished();

	UFUNCTION()
	void OnLocationDashFinished();

	UFUNCTION()
	void OnDashDamageSync();

	UPROPERTY()
	TObjectPtr<AAssassinsProjectile> ThrownProjectile;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayTagRemoved> TargetMarkWaitTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> ShroudMarkWaitTask;

	bool bProjectileHit = false;

	// Owning client: the dash to the unit dealt its damage, and the attack follows the server's end.
	bool bAttackOnEnd = false;
};
