#include "AbilitySystem/Abilities/Champions/Leblanc/Ability_Leblanc_Q.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AssassinsGameplayTags.h"
#include "Character/Champions/Leblanc/AssassinsChampionSkillState_Leblanc.h"
#include "Components/SkeletalMeshComponent.h"
#include "NativeGameplayTags.h"

namespace LeblancSigil
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_MIMIC_ABILITY1, "Event.Champion.Leblanc.Mimic.Q");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_SPAWN_PROJECTILE_NOTIFY, "Event.AnimNotify.SpawnProjectile");
};

UAbility_Leblanc_Q::UAbility_Leblanc_Q(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProjectileSpawnSocket = TEXT("SigilSocket");
}

void UAbility_Leblanc_Q::PrePlayMontage()
{
	Super::PrePlayMontage();

	if (!IsActive())
	{
		return;
	}

	AddTagToAvatar(AssassinsGameplayTags::Status_Channeling);
	UAssassinsChampionSkillState_Leblanc::SetMimicAbilityOf(*this, LeblancSigil::TAG_EVENT_MIMIC_ABILITY1);

	if (K2_HasAuthority() && !IsLocallyControlled())
	{
		UAbilityTask_WaitReplicatedEvent* SpawnRequestTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, SpawnProjectileEventType);
		SpawnRequestTask->OnEvent.AddDynamic(this, &ThisClass::OnSpawnProjectileRequested);
		SpawnRequestTask->ReadyForActivation();
	}
}

FGameplayTagContainer UAbility_Leblanc_Q::GetPendingCastTags() const
{
	return FGameplayTagContainer(AssassinsGameplayTags::Status_Channeling);
}

void UAbility_Leblanc_Q::PostPlayMontage()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	UAbilityTask_WaitGameplayEvent* NotifyTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, LeblancSigil::TAG_EVENT_SPAWN_PROJECTILE_NOTIFY, nullptr, true, true);
	NotifyTask->EventReceived.AddDynamic(this, &ThisClass::OnSpawnProjectileNotify);
	NotifyTask->ReadyForActivation();
}

void UAbility_Leblanc_Q::OnSpawnProjectileNotify(FGameplayEventData Payload)
{
	if (K2_HasAuthority())
	{
		SpawnProjectile();
		return;
	}

	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);
	ServerSetReplicatedEvent(SpawnProjectileEventType);
}

void UAbility_Leblanc_Q::OnSpawnProjectileRequested()
{
	SpawnProjectile();
}

void UAbility_Leblanc_Q::SpawnProjectile()
{
	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);

	const USkeletalMeshComponent* Mesh = GetOwningComponentFromActorInfo();
	if (!IsValid(Mesh))
	{
		return;
	}

	if (AAssassinsProjectile* Projectile = SpawnAbilityProjectile(ProjectileClass, FTransform(FRotator::ZeroRotator, Mesh->GetSocketLocation(ProjectileSpawnSocket))))
	{
		HandleProjectile(Projectile);
	}
}

void UAbility_Leblanc_Q::HandleProjectile(AAssassinsProjectile* SpawnedProjectile)
{
	if (!IsValid(AbilityTargetActor))
	{
		SpawnedProjectile->Destroy();
		return;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	SpawnedProjectile->SetVelocity((AvatarActor ? AvatarActor->GetActorForwardVector() : FVector::ForwardVector) * ProjectileSpeed);

	SpawnedProjectile->InitHomingProjectile(AbilityTargetActor, MakeEffectSpecHandle(DamageEffectClass), HomingAcceleration);
}
