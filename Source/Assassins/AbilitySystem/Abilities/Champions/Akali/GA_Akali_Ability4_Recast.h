#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GameplayEffectTypes.h"
#include "GA_Akali_Ability4_Recast.generated.h"

class UCapsuleComponent;
class UCurveTable;
class UGameplayEffect;
class UPrimitiveComponent;

UCLASS(Abstract)
class UGA_Akali_Ability4_Recast : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability4_Recast(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<TObjectPtr<AActor>> OverlappedActors;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	bool HasPlayedHitEffect = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	FGameplayEffectSpecHandle DamageSpecHandle;

private:

	void AcquireCapsule();
	void ReleaseCapsule();

	void HandleOverlappedActor(AActor* OverlappedActor);
	float CalculateDamageIncrease(AActor* Actor) const;

	UFUNCTION()
	void OnTargetHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnAvatarMoved(float DeltaSeconds, FVector OldLocation, FVector OldVelocity);

	UFUNCTION()
	void OnDashEnded();

	UFUNCTION()
	void OnPauseMontageNotify(FGameplayEventData Payload);

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> AbilityCapsule;
};
