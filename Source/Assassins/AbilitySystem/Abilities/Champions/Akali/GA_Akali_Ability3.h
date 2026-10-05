#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GA_Akali_Ability3.generated.h"

class AAssassinsProjectile;
class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayTagRemoved;
class UCurveTable;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Akali_Ability3 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability3(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double FlipSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double FlipDistance = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double DashSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> ProjectileDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DashDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> DashMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UCurveTable> AbilityMagnitudeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName DashMarkDurationRow;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AActor> ProjectileHitTarget;

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

	bool bAttackOnEnd = false;
};
