#include "Character/Champions/Akali/AkaliShroud.h"

#include "AbilitySystem/AssassinsProjectile.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"

AAkaliShroud::AAkaliShroud()
{
	PrimaryActorTick.bCanEverTick = true;

	DashMarkLocation = FVector::ZeroVector;
}

void AAkaliShroud::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	HandleOwnerProjectile();
}

void AAkaliShroud::IsActorInStealthRadius(AActor* TargetActor, bool& CanStealth) const
{
	CanStealth = false;

	if (!IsValid(TargetActor))
	{
		return;
	}

	const double DistanceFromOwner = FVector::Dist2D(TargetActor->GetActorLocation(), GetActorLocation());
	CanStealth = (DistanceFromOwner <= OuterRadius) && (DistanceFromOwner >= InnerRadius);
}

void AAkaliShroud::HandleOwnerProjectile()
{
	AActor* OwnerActor = GetOwner();
	if ((OwnerActor == nullptr) || !OwnerActor->HasAuthority() || !bCanHandleProjectile)
	{
		return;
	}

	UAssassinsChampionSkillState_Akali* AkaliState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Akali>(OwnerActor);
	if (AkaliState == nullptr)
	{
		Destroy();
		return;
	}

	AAssassinsProjectile* Projectile = AkaliState->SpawnedProjectile;
	if (!IsValid(Projectile))
	{
		return;
	}

	bool bInRing = false;
	IsActorInStealthRadius(Projectile, bInRing);
	if (!bInRing)
	{
		return;
	}

	DashMarkLocation = CalculateDashMarkLocation(Projectile);
	ClientReceiveDashMarkLocation(DashMarkLocation);

	bCanHandleProjectile = false;
	Projectile->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnHandledProjectileDestroyed);

	Projectile->HandleTargetOverlap(this, DashMarkLocation);
}

FVector AAkaliShroud::CalculateDashMarkLocation(const AActor* Projectile) const
{
	const FVector ShroudLocation = GetActorLocation();
	const FVector ToProjectile = (Projectile->GetActorLocation() - ShroudLocation).GetSafeNormal2D(1.e-4);

	return ShroudLocation + (ToProjectile * ((InnerRadius + OuterRadius) / 2.0));
}

void AAkaliShroud::ClientReceiveDashMarkLocation_Implementation(FVector MarkLocation)
{
	DashMarkLocation = MarkLocation;
}

void AAkaliShroud::OnHandledProjectileDestroyed(AActor* DestroyedActor)
{
	bCanHandleProjectile = true;
}
