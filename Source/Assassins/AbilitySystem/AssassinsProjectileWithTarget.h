#pragma once

#include "AbilitySystem/AssassinsProjectile.h"
#include "AssassinsProjectileWithTarget.generated.h"

UCLASS()
class ASSASSINS_API AAssassinsProjectileWithTarget : public AAssassinsProjectile
{
	GENERATED_BODY()

public:

	virtual void InitHomingProjectile_Implementation(AActor* ProjectileTarget, const FGameplayEffectSpecHandle& DamageContextHandle, double HomingAcceleration) override;
	virtual void HandleTargetOverlap_Implementation(AActor* TargetActor, const FVector& ImpactLocation) override;

	virtual bool CanHitCheckTarget(AActor* TargetActor) const override;

protected:

	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	TObjectPtr<AActor> AttackTarget;
};
