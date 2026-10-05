// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "GA_Zed_Ability2.generated.h"

class AAssassinsZedShadow;
class UParticleSystem;

/**
 * UGA_Zed_Ability2
 *
 * Zed W, Living Shadow. Zed projects a shadow to the cursor; pressing the ability again swaps him with it.
 * The ability lasts as long as the shadow. Was the GA_Zed_Ability2 blueprint.
 */
UCLASS(Abstract)
class UGA_Zed_Ability2 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Zed_Ability2(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	TSubclassOf<AAssassinsZedShadow> ShadowClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ShadowLifeSpan = 0.0;

	// How fast the shadow travels to the cursor: it appears once it would have got there.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ShadowProjectionSpeed = 0.0;

	// Attached to Zed as he casts.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UParticleSystem> CastParticle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AAssassinsZedShadow> SpawnedShadow;

private:

	void SpawnShadow();
	void WaitForRecast();
	void MoveToShadow();
	double GetShadowProjectionTime(const FVector& ProjectionLocation) const;

	UFUNCTION()
	void OnShadowEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UFUNCTION()
	void OnRecastReleased(float TimeHeld);

	UFUNCTION()
	void OnRecastPressed(float TimeWaited);

	FTimerHandle ShadowSpawnTimerHandle;
};
