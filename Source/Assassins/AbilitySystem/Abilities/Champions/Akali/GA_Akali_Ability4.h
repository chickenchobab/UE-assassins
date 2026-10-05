// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_UnitTargeted.h"
#include "GameplayEffectTypes.h"
#include "GA_Akali_Ability4.generated.h"

class UCapsuleComponent;
class UGameplayEffect;
class UPrimitiveComponent;

/**
 * UGA_Akali_Ability4
 *
 * Akali R, Perfect Execution. Akali dashes through the target champion, damaging every enemy on the way. Hitting the
 * target plays a montage of its own. A while after the dash she may recast(GA_Akali_Ability4_Recast), until the
 * time to recast is up.
 *
 * The server finds the hits with the ultimate capsule of Akali, which the recast uses too: each holds it while it
 * dashes, and binds and unbinds only its own handler. The owning client watches for the target with its own copy of
 * the capsule and plays the hit as it sees it, without waiting a round trip for the server: what the dash keeps off
 * until the hit(E, W) is free as the player sees the hit, as without lag. It tells the server, which takes the hit if
 * the target is still along the dash and its own capsule did not find it first. A hit the client's capsule missed
 * comes from the server. Either way the hit montage plays in the window of the only sync point after the arrival. Was
 * the GA_Akali_Ability4 blueprint, whose end turned the capsule off and unbound every handler, the recast's included.
 */
UCLASS(Abstract)
class UGA_Akali_Ability4 : public UGA_UnitTargeted
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability4(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_UnitTargeted interface
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual void OnMontageComplete() override {}
	virtual void OnMontageCanceled() override {}
	virtual FGameplayTagContainer GetPendingCastTags() const override;
	//~End of UGA_UnitTargeted interface

protected:

	// How far past the target the dash goes.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float DashDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float DashSpeed = 0.0f;

	// After the dash, how long until Akali may recast.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float RecastDelay = 0.0f;

	// After the dash, how long the recast stays allowed. The ability ends with it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	float RecastPermissionTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Played once the dash hits the target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UAnimMontage> HitDashMontage;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector DashStartLocation = FVector::ZeroVector;

	// From the start of the dash to the target, on the ground.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FVector ToTarget = FVector::ZeroVector;

	// Server: every enemy is hit once.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<TObjectPtr<AActor>> OverlappedActors;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool HasHitAbilityTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool HasServerNotifiedClientHitEvent = false;

	// Carries where the cursor points to the recast.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectContextHandle RecastEventContext;

private:

	// Holds the ultimate capsule during the dash: the server to hit, the owning client to see the target's hit coming.
	void AcquireCapsule();
	void ReleaseCapsule();

	void HandleOverlappedActor(AActor* TargetActor);

	// Owning client: the target's hit, as its capsule sees it.
	void PredictTargetHit();

	// Server: whether the target stands along the dash, within the reach of the capsule and HitPredictionTolerance.
	bool IsTargetAlongDash() const;

	// Both sides, once the target is hit.
	void PlayHitDash();

	// Owning client.
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

	// The reach of the capsule, kept for the hit the owning client tells of, which may come after the dash.
	double CapsuleRadius = 0.0;
};
