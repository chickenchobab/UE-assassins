#pragma once

#include "AbilitySystem/Abilities/AssassinsGameplayAbility.h"
#include "AITypes.h"
#include "GameplayTagContainer.h"
#include "Navigation/PathFollowingComponent.h"
#include "GA_UnitTargeted.generated.h"

class UAnimMontage;
class UAssassinsMontageWithTiming;
class UAssassinsTargetChasingComponent;

UCLASS(Abstract)
class UGA_UnitTargeted : public UAssassinsGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_UnitTargeted(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void MoveToTarget();

	virtual void PlayMontage();
	virtual void PrePlayMontage();
	virtual void PostPlayMontage() {}
	virtual void OnMontageComplete();
	virtual void OnMontageCanceled();

	virtual void SetMontageToPlay();

	virtual bool ShouldKeepFollowingTarget() const;

	virtual FGameplayTagContainer GetPendingCastTags() const;

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void StopControllerMove();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Ability")
	void ResetTargetState();

	UFUNCTION(BlueprintPure, Category = "Assassins|Ability")
	FRotator GetTargetLookAtRotation(const AActor* TargetActor) const;

	void WaitForNetSync();

	UFUNCTION()
	virtual void OnMoveComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	UFUNCTION()
	virtual void OnServerReachedToTarget();

	UFUNCTION()
	virtual void OnNetSync();

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageCanceled();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double AbilityRange = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	bool ShouldTargetChampion = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<FGameplayTag> ActionBlockingTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<TObjectPtr<UAssassinsMontageWithTiming>> MontageDataArray;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double MontageRate = 1.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> MontageToPlay;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	double HitEventTime = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AActor> AbilityTargetActor;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAssassinsTargetChasingComponent> CachedTargetChasingComponent;

private:

	bool IsTargetInRange() const;

	void AddPendingCastTags();
	void RemovePendingCastTags();

	bool bPendingCastTagsAdded = false;
};
