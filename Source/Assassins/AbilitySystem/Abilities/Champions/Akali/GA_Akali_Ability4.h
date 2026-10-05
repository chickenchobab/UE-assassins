#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_UnitTargeted.h"
#include "GameplayEffectTypes.h"
#include "GA_Akali_Ability4.generated.h"

class UCapsuleComponent;
class UGameplayEffect;
class UPrimitiveComponent;

UCLASS(Abstract)
class UGA_Akali_Ability4 : public UGA_UnitTargeted
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability4(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector DashStartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector ToTarget = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<TObjectPtr<AActor>> OverlappedActors;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool HasHitAbilityTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool HasServerNotifiedClientHitEvent = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectContextHandle RecastEventContext;

private:

	void AcquireCapsule();
	void ReleaseCapsule();

	void HandleOverlappedActor(AActor* TargetActor);

	void PredictTargetHit();

	bool IsTargetAlongDash() const;

	void PlayHitDash();

	void WaitAndHandleRecast();
	void Recast();

	UFUNCTION()
	void OnTargetHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void ClientGetNotifiedOfHitEvent();

	UFUNCTION()
	void OnClientPredictedHit();

	UFUNCTION()
	void OnHitDashSynced();

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

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> AbilityCapsule;

	double CapsuleRadius = 0.0;
};
