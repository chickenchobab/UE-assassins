#pragma once

#include "AbilitySystem/Abilities/Foundation/Ability_LocationTargeted.h"
#include "GameplayEffectTypes.h"
#include "GameplayPrediction.h"
#include "GameplayTagContainer.h"
#include "Ability_Leblanc_W.generated.h"

class UAbilityTask_DashTo;
class UAbilityTask_WaitDelay;
class UGameplayEffect;

UCLASS(Abstract)
class UAbility_Leblanc_W : public UAbility_LocationTargeted
{
	GENERATED_BODY()

public:

	UAbility_Leblanc_W(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float DashSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float ReturnDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayTag ReturnLocationCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> ReturnMontage;

	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpecHandle;

	UPROPERTY()
	FVector ReturnLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator ReturnRotation = FRotator::ZeroRotator;

	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;

private:

	bool IsMimic() const;

	void HandleDashComplete(const FVector& EndLocation);
	void ApplyDashDamage();

	void CheckLanding();
	void ApplyLandingHits();

	void ApplyHit(AActor* Target);

	void WaitForRecast();

	void WaitForRecastInputRelease();
	void WaitForReturnPress(bool bTestAlreadyPressed);

	void DoReturn();
	void FinishDashEarly();
	void StartReturn();

	UFUNCTION()
	void OnDashFinished();

	UFUNCTION()
	void OnDashCancelled();

	UFUNCTION()
	void OnDashSynced();

	UFUNCTION()
	void OnRecastInputReleased(float TimeHeld);

	UFUNCTION()
	void OnRecastInputPressed();

	UFUNCTION()
	void OnUltimateRecast(FGameplayEventData Payload);

	UFUNCTION()
	void OnReturnRequested();

	UFUNCTION()
	void OnReturnDurationFinished();

	UFUNCTION()
	void OnReturnMontageEnded();

	UPROPERTY()
	TObjectPtr<UAbilityTask_DashTo> DashTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> ReturnDurationTask;

	FVector DashEndLocation = FVector::ZeroVector;

	TArray<TWeakObjectPtr<AActor>> LandingHits;
	bool bLandingChecked = false;
	FPredictionKey SyncPredictionKey;

	int32 ActivationNumber = 0;

	bool bDashFinished = false;
	bool bDashSynced = false;

	bool bRecastInputReleased = false;

	bool bReturnRequested = false;
	bool bReturnStarted = false;
};
