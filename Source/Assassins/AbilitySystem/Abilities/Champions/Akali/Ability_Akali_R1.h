#pragma once

#include "AbilitySystem/Abilities/Foundation/Ability_UnitTargeted.h"
#include "GameplayEffectTypes.h"
#include "Ability_Akali_R1.generated.h"

class UGameplayEffect;

UCLASS(Abstract)
class UAbility_Akali_R1 : public UAbility_UnitTargeted
{
	GENERATED_BODY()

public:

	UAbility_Akali_R1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCanceled() override {}
	virtual FGameplayTagContainer GetPendingCastTags() const override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float DashDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float DashSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float RecastDelay = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float RecastPermissionTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> HitDashMontage;

	UPROPERTY()
	FVector DashStartLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector ToTarget = FVector::ZeroVector;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	UPROPERTY()
	bool HasHitAbilityTarget = false;

	UPROPERTY()
	bool HasServerNotifiedClientHitEvent = false;

	UPROPERTY()
	bool HasDashBeenCut = false;

	UPROPERTY()
	FGameplayEffectContextHandle RecastEventContext;

private:

	void BeginHits();
	void EndHits();

	void HandleHit(AActor* TargetActor);

	void PredictTargetHit();

	bool IsTargetAlongDash() const;

	void PlayHitDash();

	void WaitAndHandleRecast();
	void Recast();

	UFUNCTION()
	void OnAvatarMoved(float DeltaSeconds, FVector OldLocation, FVector OldVelocity);

	UFUNCTION()
	void ClientGetNotifiedOfHitEvent();

	UFUNCTION()
	void OnClientPredictedHit();

	UFUNCTION()
	void OnHitDashSynced();

	UFUNCTION()
	void OnDashCancelled();

	UFUNCTION()
	void OnDashEnded();

	UFUNCTION()
	void OnRecastAllowed();

	UFUNCTION()
	void OnRecastPermissionEnded();

	UFUNCTION()
	void OnRecastInputReleased(float TimeHeld);

	UFUNCTION()
	void OnRecastInputPressed(float TimeWaited);

	int32 HitShapeHandle = 0;

	FVector TargetLocationBefore = FVector::ZeroVector;
};
