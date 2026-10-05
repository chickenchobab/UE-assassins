#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "GA_Akali_Ability2.generated.h"

class AAssassinsShroud;
class UCurveTable;
class UGameplayEffect;

UCLASS(Abstract)
class UGA_Akali_Ability2 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability2(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsShroud> ShroudClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> StealthEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DisableStealthEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UCurveTable> AbilityMagnitudeTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName ShroudDurationRow;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AAssassinsShroud> SpawnedShroud;

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
