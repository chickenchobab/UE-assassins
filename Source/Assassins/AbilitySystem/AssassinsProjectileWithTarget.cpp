#include "AbilitySystem/AssassinsProjectileWithTarget.h"

void AAssassinsProjectileWithTarget::InitHomingProjectile_Implementation(AActor* ProjectileTarget, const FGameplayEffectSpecHandle& DamageContextHandle, double HomingAcceleration)
{
	if (!IsValid(ProjectileTarget))
	{
		Destroy();
		return;
	}

	AttackTarget = ProjectileTarget;
	DamageSpecHandle = DamageContextHandle;

	EnableHoming(AttackTarget->GetRootComponent(), static_cast<float>(HomingAcceleration));
	FinishInitProjectile();
}

void AAssassinsProjectileWithTarget::HandleTargetOverlap_Implementation(AActor* TargetActor, const FVector& ImpactLocation)
{
	if (TargetActor != AttackTarget)
	{
		return;
	}

	ApplyGameplayEffectSpecToTargetActor(DamageSpecHandle, AttackTarget);
	ShouldBeDestroyed = true;
	SetLifeSpan(0.1f);
}

bool AAssassinsProjectileWithTarget::CanHitCheckTarget(AActor* TargetActor) const
{
	return IsValid(AttackTarget) && (TargetActor == AttackTarget);
}

void AAssassinsProjectileWithTarget::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && !IsValid(AttackTarget))
	{
		Destroy();
	}
}
