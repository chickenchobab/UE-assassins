#pragma once

#include "AbilitySystem/Abilities/Foundation/Ability_LocationTargeted.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "Ability_Akali_W.generated.h"

class AAkaliShroud;
class UCurveTable;
class UGameplayEffect;

UCLASS(Abstract)
class UAbility_Akali_W : public UAbility_LocationTargeted
{
	GENERATED_BODY()

public:

	UAbility_Akali_W(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAkaliShroud> ShroudClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> StealthEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DisableStealthEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UCurveTable> AbilityMagnitudeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName ShroudDurationRow;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AAkaliShroud> SpawnedShroud;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FActiveGameplayEffectHandle StealthEffectHandle;

private:

	void SpawnShroud();
	void InitShroud();

	void UpdateStealth();
	void ApplyStealth();
	void FinishStealth();
	void BreakStealth();
	bool IsStealthActive() const;

	UFUNCTION()
	void OnStealthTick(float DeltaTime);

	UFUNCTION()
	void OnOtherAbilityActivated(UGameplayAbility* ActivatedAbility);

	UFUNCTION()
	void OnAttackStarted();

	UFUNCTION()
	void OnShroudEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	FTimerHandle ShroudSpawnTimerHandle;
};
