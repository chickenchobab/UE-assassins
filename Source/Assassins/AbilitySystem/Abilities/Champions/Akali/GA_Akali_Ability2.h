// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "GA_Akali_Ability2.generated.h"

class AAssassinsShroud;
class UCurveTable;
class UGameplayEffect;

/**
 * UGA_Akali_Ability2
 *
 * Akali W, Twilight Shroud. Akali drops a shroud at the cursor; inside its ring she is stealthed. Casting an ability
 * or attacking breaks the stealth for a moment. The ability lasts as long as the shroud.
 *
 * The stealth is decided by the server alone: enemies see what the server says anyway, and the owning client gets
 * the stealth effect replicated. Was the GA_Akali_Ability2 blueprint, which had the client drive it through sync
 * points that piled up and applied the stealth more than once.
 */
UCLASS(Abstract)
class UGA_Akali_Ability2 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability2(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_LocationTargeted_Immediate interface
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}
	//~End of UGA_LocationTargeted_Immediate interface

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsShroud> ShroudClass;

	// Infinite: removed when Akali leaves the ring or the stealth breaks.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> StealthEffectClass;

	// Keeps Akali from stealthing again for a moment after she broke it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DisableStealthEffectClass;

	// How long the shroud lasts, by ability level.
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

	// Server: stealth while Akali is in the ring and nothing keeps her from it.
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
