// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_UnitTargeted.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "GA_Attack.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * UGA_Attack
 *
 * The basic attack. The avatar walks up to its target, plays an attack montage and hits at the timing that comes
 * with the montage. It does not commit: the attack is also used to reach the target, and the ability that keeps
 * attacking(Ability.ActivateAttack) is what drives it. While it runs the click is watched, so that picking another
 * target starts the attack over on that one. Was the GA_Attack blueprint.
 */
UCLASS(Abstract)
class UGA_Attack : public UGA_UnitTargeted
{
	GENERATED_BODY()

public:

	UGA_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_UnitTargeted interface
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
	//~End of UGA_UnitTargeted interface

	// The moment the attack lands: the melee and the ranged attack deal their damage from here.
	virtual void OnAttackHit();

	// The effect the attack applies to what it hits.
	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	virtual TSubclassOf<UGameplayEffect> GetAttackEffectClass() const;

	// Rate a montage has to play at to fit in the attack cooldown that is left.
	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	double GetMontagePlayRate(UAnimMontage* Montage) const;

	// Asks for an attack on TargetActor: the ability that keeps attacking answers the event, where the avatar is controlled.
	void SendActivateAttackEvent(AActor* TargetActor);

	// Stops the ability that keeps attacking, so that it can be started again on another target.
	void CancelActivateAttack();

	// Fits the montage in the attack speed and moves the hit along with it.
	void AdjustMontageRateAndEventTime();

private:

	// Tells the chasing component whether the avatar keeps following the target once the attack is done.
	void UpdateTargetChasingCondition();

	// Waits for the click to be released and pressed again: that is when another target may have been picked.
	void GetReadyToTargetChange();

	void HandleAttackTargetChanged(AActor* NewTarget);

	// Both ends of the montage leave the attack the same way.
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

	// Paces the attacks. Its remaining duration is also what the montage is made to fit in.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> AttackCooldownEffectClass;

	// What the attack applies to its target, unless GetAttackEffectClass says otherwise.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> AttackEffectClass;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FActiveGameplayEffectHandle AttackCooldownActiveHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool IsMontageEnded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle AttackDamageSpecHandle;

private:

	// The target the player picked while the attack was running, sent one tick later.
	UPROPERTY()
	TObjectPtr<AActor> PendingAttackTarget;

	FTimerHandle AttackHitTimerHandle;
};
