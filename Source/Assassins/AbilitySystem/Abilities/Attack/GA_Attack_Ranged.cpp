#include "AbilitySystem/Abilities/Attack/GA_Attack_Ranged.h"

#include "AbilitySystem/AssassinsProjectile.h"

void UGA_Attack_Ranged::OnAttackHit()
{
	// 투사체가 복제되므로 서버만 던진다. 클라이언트도 던지면 자기 사본이 옆에 하나 더 보인다.
	if (K2_HasAuthority())
	{
		SpawnProjectile();
	}

	SendActivateAttackEvent(AbilityTargetActor);

	Super::OnAttackHit();

	if (IsMontageEnded)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Attack_Ranged::SpawnProjectile()
{
	IAssassinsProjectileHandler::Execute_SetProjectileClass(this);

	FTransform SpawnTransform;
	IAssassinsProjectileHandler::Execute_SetProjectileSpawnTransform(this, GetAvatarActorFromActorInfo(), SpawnTransform);

	AAssassinsProjectile* SpawnedProjectile = SpawnAbilityProjectile(ProjectileClass, SpawnTransform);
	IAssassinsProjectileHandler::Execute_HandleProjectile(this, SpawnedProjectile);
}

void UGA_Attack_Ranged::SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform)
{
	if (IsValid(SourceActor))
	{
		SpawnTransform = SourceActor->GetTransform();
	}
}

void UGA_Attack_Ranged::HandleProjectile_Implementation(AAssassinsProjectile* SpawnedProjectile)
{
	if (SpawnedProjectile == nullptr)
	{
		return;
	}

	if (!IsValid(AbilityTargetActor))
	{
		SpawnedProjectile->Destroy();
		return;
	}

	SpawnedProjectile->InitHomingProjectile(AbilityTargetActor, MakeEffectSpecHandle(GetAttackEffectClass()), HomingAcceleration);
}
