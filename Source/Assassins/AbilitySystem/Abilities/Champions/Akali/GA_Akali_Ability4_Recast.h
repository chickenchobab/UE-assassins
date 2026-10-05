// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GameplayEffectTypes.h"
#include "GA_Akali_Ability4_Recast.generated.h"

class UCapsuleComponent;
class UCurveTable;
class UGameplayEffect;
class UPrimitiveComponent;

/**
 * UGA_Akali_Ability4_Recast
 *
 * The recast of Akali R. Triggered by the event of the ultimate, Akali dashes the way the cursor points, damaging
 * every enemy on the way the more health it misses.
 *
 * The server finds the hits with the ultimate capsule of Akali, held for the dash alongside the ultimate, whose end
 * may come right after this starts. Besides its overlaps, the capsule is swept along every move of Akali: a long move
 * (a slow frame) could take it past an enemy it overlaps neither before nor after. Was the GA_Akali_Ability4_Recast
 * blueprint, which the end of the ultimate could leave without the capsule.
 */
UCLASS(Abstract)
class UGA_Akali_Ability4_Recast : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability4_Recast(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
	float DashSpeed = 0.0f;

	// Takes the multiplier of the damage by SetByCaller.Akali.DamageMultiplication.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// The multiplier of the damage by the missing health of the target, from 0 to 1.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UCurveTable> DamagePerMissingHealthTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName DamagePerMissingHealthRow;

	// Server: every enemy is hit once.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<TObjectPtr<AActor>> OverlappedActors;

	// The hit effect plays for the first hit only.
	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool HasPlayedHitEffect = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle DamageSpecHandle;

private:

	// Server: holds the ultimate capsule during the dash.
	void AcquireCapsule();
	void ReleaseCapsule();

	void HandleOverlappedActor(AActor* OverlappedActor);
	float CalculateDamageIncrease(AActor* Actor) const;

	UFUNCTION()
	void OnTargetHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Server: sweeps the capsule from where the move of Akali started to where it ended.
	UFUNCTION()
	void OnAvatarMoved(float DeltaSeconds, FVector OldLocation, FVector OldVelocity);

	UFUNCTION()
	void OnDashEnded();

	UFUNCTION()
	void OnPauseMontageNotify(FGameplayEventData Payload);

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> AbilityCapsule;
};
