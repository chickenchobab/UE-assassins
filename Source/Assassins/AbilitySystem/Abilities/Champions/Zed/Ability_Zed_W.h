#pragma once

#include "AbilitySystem/Abilities/Foundation/Ability_LocationTargeted.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "Ability_Zed_W.generated.h"

class AZedShadow;
class UParticleSystem;

UCLASS(Abstract)
class UAbility_Zed_W : public UAbility_LocationTargeted
{
	GENERATED_BODY()

public:

	UAbility_Zed_W(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AZedShadow> ShadowClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ShadowLifeSpan = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ShadowProjectionSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UParticleSystem> CastParticle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<AZedShadow> SpawnedShadow;

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
