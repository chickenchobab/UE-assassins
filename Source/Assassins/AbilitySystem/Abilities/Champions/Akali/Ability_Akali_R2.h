#pragma once

#include "AbilitySystem/Abilities/Foundation/Ability_LocationTargeted.h"
#include "GameplayEffectTypes.h"
#include "Ability_Akali_R2.generated.h"

class UCurveTable;
class UGameplayEffect;

UCLASS(Abstract)
class UAbility_Akali_R2 : public UAbility_LocationTargeted
{
	GENERATED_BODY()

public:

	UAbility_Akali_R2(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCancelled() override {}

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float DashSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UCurveTable> DamagePerMissingHealthTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName DamagePerMissingHealthRow;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	UPROPERTY()
	bool HasPlayedHitEffect = false;

	UPROPERTY()
	FGameplayEffectSpecHandle DamageSpecHandle;

private:

	void BeginHits();
	void EndHits();

	void HandleCheckedHit(AActor* Target, int32 CheckedActivation);

	void ApplyHit(AActor* Target);
	float CalculateDamageIncrease(AActor* Actor) const;

	UFUNCTION()
	void OnDashEnded();

	UFUNCTION()
	void OnPauseMontageNotify(FGameplayEventData Payload);

	int32 HitShapeHandle = 0;

	int32 ActivationNumber = 0;
};
