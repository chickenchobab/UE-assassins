// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_UnitTargeted.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "GA_Zed_Ability4.generated.h"

class AAssassinsZedShadow;
class UAssassinsHealthComponent;
class UGameplayEffect;

/**
 * UGA_Zed_Ability4
 *
 * Zed ultimate, Death Mark. Zed leaves a shadow where he stands, vanishes and reappears behind the target, which he
 * marks: after a delay the mark bursts with the damage Zed dealt to the target in the meantime. Pressing the ability
 * again swaps Zed with the shadow. Was the GA_Zed_Ability4 blueprint.
 */
UCLASS(Abstract)
class UGA_Zed_Ability4 : public UGA_UnitTargeted
{
	GENERATED_BODY()

public:

	UGA_Zed_Ability4(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_UnitTargeted interface
	virtual void PrePlayMontage() override;
	virtual void OnMontageComplete() override;
	virtual void OnMontageCanceled() override;
	virtual void SetMontageToPlay() override {}
	virtual FGameplayTagContainer GetPendingCastTags() const override;
	//~End of UGA_UnitTargeted interface

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsZedShadow> ShadowClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ShadowLifeSpan = 0.0;

	// How long after Zed reappears the mark bursts.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double DamageEffectDelay = 0.0;

	// The burst. Its magnitude is the damage stored meanwhile(SetByCaller.Zed.StoredDamage).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Played as Zed reappears behind the target.
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

	// The ultimate is over once the mark burst and Zed went back to the shadow(or can no longer). The server decides it,
	// and its end reaches the owning client.
	void EndIfDone();

	// The delegates this ability binds on other objects, which outlive it.
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

	// The mark is set and has not burst yet.
	bool bDetonationPending = false;
};
