#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_UnitTargeted.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "GA_Zed_Ability4.generated.h"

class AAssassinsZedShadow;
class UAssassinsHealthComponent;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Zed_Ability4 : public UGA_UnitTargeted
{
	GENERATED_BODY()

public:

	UGA_Zed_Ability4(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void OnMontageComplete() override;
	virtual void OnMontageCanceled() override;
	virtual void SetMontageToPlay() override {}
	virtual FGameplayTagContainer GetPendingCastTags() const override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsZedShadow> ShadowClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ShadowLifeSpan = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double DamageEffectDelay = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> StrikeMontage;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	double StoredDamage = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool HasMovedToShadow = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool IsDamageDone = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AAssassinsZedShadow> SpawnedShadow;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle DamageSpecHandle;

private:

	void SpawnShadow();
	void TeleportToTarget();
	void MoveToShadow();
	void SetCapsuleCollisionEnabled(bool bCollisionEnabled);
	void StartMark();
	void EffectEvent();
	void DeathMarkDetonation();

	void EndIfDone();

	void UnbindChannelingEnded();
	void UnbindMarkedHealth();
	void UnbindShadow();

	UFUNCTION()
	void AbortPausedMovement();

	UFUNCTION()
	void OnShadowEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UFUNCTION()
	void StoreDamage(UAssassinsHealthComponent* ChangedHealthComponent, float OldValue, float NewValue, AActor* DamageInstigator);

	UFUNCTION()
	void OnStrikeCompleted();

	UFUNCTION()
	void OnRecastReleased(float TimeHeld);

	UFUNCTION()
	void OnRecastPressed(float TimeWaited);

	UFUNCTION()
	void OnDetonationSync();

	TWeakObjectPtr<UAssassinsHealthComponent> MarkedHealthComponent;

	FTimerHandle EffectEventTimerHandle;
	FTimerHandle DetonationTimerHandle;

	bool bDetonationPending = false;
};
