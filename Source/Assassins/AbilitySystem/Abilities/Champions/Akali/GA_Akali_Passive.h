// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_Akali_Passive.generated.h"

class UAbilityTask_CustomizeTickTask;
class UAbilityTask_WaitDelay;
class UAnimMontage;
class UCurveTable;
class UGameplayEffect;

/**
 * UGA_Akali_Passive
 *
 * Akali passive, Assassin's Mark. When an ability of Akali damages an enemy champion, a ring appears around it.
 * Leaving the ring powers up her next attack(Status.Combo.Passive). She runs faster toward the edge while she is in
 * the ring, and toward enemy champions while the attack is powered up.
 *
 * The server alone decides all of it: the ring, the passive and the speed are what enemies see anyway, and the owning
 * client gets the effects and the ring cue replicated. The owning client only plays the passive montage, since it
 * does not take the server's. Was the GA_Akali_Passive blueprint, which synced with the client every frame for the
 * speed, and had the client spawn a ring of its own from a location sent on another channel.
 */
UCLASS(Abstract)
class UGA_Akali_Passive : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_Akali_Passive(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

protected:

	// Enemy champions this close count for the speed of the powered-up passive.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float EnemyDetectionRadius = 0.0f;

	// How far off the way to an enemy champion Akali may run and still be running toward it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float MoveTowardDegree = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float RingRadius = 0.0f;

	// Grants Status.Champion.Akali.CanGainSpeed for a while: once the ring appears, and again when the passive comes on.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> CanGainSpeedEffectClass;

	// Grants Status.Combo.Passive, which powers up the next attack.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> PassiveStatusEffectClass;

	// Infinite: removed as soon as Akali stops running the way that gains speed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> GainSpeedEffectClass;

	// How long the ring lasts, by ability level.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UCurveTable> AbilityMagnitudeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName RingDurationRow;

	// Played as the passive comes on, unless Akali is in the middle of an ability.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> PassiveMontage;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector RingLocation = FVector::ZeroVector;

	// The speed this ability applied, while Akali gains speed.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FActiveGameplayEffectHandle SpeedEffectHandle;

private:

	// Server: a new ring replaces the old one.
	void SpawnRing();
	void ClearRingTasks();
	void ActivatePassive();
	void PlayPassiveMontage();

	// Server: speed while Akali runs out of the ring, or toward an enemy champion once the passive is on.
	void UpdateSpeed();
	void RemoveSpeed();
	bool IsSpeedEffectActive() const;

	bool IsAvatarOutOfRingRadius() const;
	bool CanPlayPassiveActivatingMontage() const;
	bool IsAvatarGettingOutOfRing() const;
	bool IsAvatarMovingTowardTarget(const AActor* TargetActor) const;
	bool IsAvatarMovingTowardEnemy() const;

	UFUNCTION()
	void OnSpawnRingEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnRingTick(float DeltaTime);

	UFUNCTION()
	void OnRingDurationFinished();

	UFUNCTION()
	void OnSpeedTick(float DeltaTime);

	UFUNCTION()
	void OnPassiveStatusAdded();

	UPROPERTY()
	TObjectPtr<UAbilityTask_CustomizeTickTask> RingDistanceCheckingTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> RingDurationTask;
};
