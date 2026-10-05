#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_UnitTargeted.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "GA_Attack.generated.h"

class UAnimMontage;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Attack : public UGA_UnitTargeted
{
	GENERATED_BODY()

public:

	UGA_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void OnMoveComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result) override;
	virtual void OnServerReachedToTarget() override;
	virtual void OnNetSync() override;
	virtual void PlayMontage() override;
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override;
	virtual void OnMontageCanceled() override;
	virtual void SetMontageToPlay() override;
	virtual bool ShouldKeepFollowingTarget() const override;

	virtual void OnAttackHit();

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	virtual TSubclassOf<UGameplayEffect> GetAttackEffectClass() const;

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	double GetMontagePlayRate(UAnimMontage* Montage) const;

	void SendActivateAttackEvent(AActor* TargetActor);

	void CancelActivateAttack();

	void AdjustMontageRateAndEventTime();

private:

	void UpdateTargetChasingCondition();

	void GetReadyToTargetChange();

	void HandleAttackTargetChanged(AActor* NewTarget);

	void HandleMontageEnded();

	void SendPendingAttackEvent();

	void OnHitTimerElapsed();

	UFUNCTION()
	void OnChasingTick(float DeltaTime);

	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	UFUNCTION()
	void OnInputPressed(float TimeWaited);

	UFUNCTION()
	void OnHitNetSync();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> AttackCooldownEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> AttackEffectClass;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FActiveGameplayEffectHandle AttackCooldownActiveHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool IsMontageEnded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle AttackDamageSpecHandle;

private:

	UPROPERTY()
	TObjectPtr<AActor> PendingAttackTarget;

	FTimerHandle AttackHitTimerHandle;
};
