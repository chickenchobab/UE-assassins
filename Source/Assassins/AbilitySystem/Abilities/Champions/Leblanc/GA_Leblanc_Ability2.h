#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GA_Leblanc_Ability2.generated.h"

class UAbilityTask_DashTo;
class UAbilityTask_WaitDelay;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Leblanc_Ability2 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Ability2(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle DamageSpecHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector ReturnLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FRotator ReturnRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TSet<TObjectPtr<AActor>> ActorsToIgnore;

private:

	bool IsMimic() const;

	void HandleDashComplete(const FVector& EndLocation);
	void ApplyDashDamage();

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

	bool bDashFinished = false;
	bool bDashSynced = false;

	bool bRecastInputReleased = false;

	bool bReturnRequested = false;
	bool bReturnStarted = false;
};
