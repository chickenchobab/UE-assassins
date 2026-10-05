#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GA_Leblanc_Ability3.generated.h"

class AAssassinsProjectile;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Leblanc_Ability3 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Ability3(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override;
	virtual void OnMontageCancelled() override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileRange = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double TetherRange = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> IdleMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> RunMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName ProjectileSpawnSocket;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AActor> ChainTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle DamageEffectSpecHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayTag TetherEffectTag;

private:

	void AdjustToMovement();

	void SpawnProjectile();
	void HandleProjectile(AAssassinsProjectile* SpawnedProjectile);
	void HandleChainHit();
	void EndIfNothingPending();
	void CheckChainMissed();
	void BreakTether();

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
