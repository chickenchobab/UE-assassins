#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_Akali_Passive.generated.h"

class UAbilityTask_CustomizeTickTask;
class UAbilityTask_WaitDelay;
class UAnimMontage;
class UCurveTable;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Akali_Passive : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_Akali_Passive(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float EnemyDetectionRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float MoveTowardDegree = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float RingRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> CanGainSpeedEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> PassiveStatusEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> GainSpeedEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UCurveTable> AbilityMagnitudeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName RingDurationRow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> PassiveMontage;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector RingLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FActiveGameplayEffectHandle SpeedEffectHandle;

private:

	void SpawnRing();
	void ClearRingTasks();
	void ActivatePassive();
	void PlayPassiveMontage();

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
