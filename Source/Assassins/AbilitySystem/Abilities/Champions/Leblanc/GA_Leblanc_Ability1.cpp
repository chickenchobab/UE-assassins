#include "AbilitySystem/Abilities/Champions/Leblanc/GA_Leblanc_Ability1.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AssassinsGameplayTags.h"
#include "Character/Champions/Leblanc/AssassinsChampionSkillState_Leblanc.h"
#include "Components/SkeletalMeshComponent.h"
#include "NativeGameplayTags.h"

namespace LeblancSigil
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_MIMIC_ABILITY1, "Event.Champion.Leblanc.Mimic.Ability1");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_SPAWN_PROJECTILE_NOTIFY, "Event.AnimNotify.SpawnProjectile");
};

UGA_Leblanc_Ability1::UGA_Leblanc_Ability1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProjectileSpawnSocket = TEXT("SigilSocket");
}

void UGA_Leblanc_Ability1::PrePlayMontage()
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

FGameplayTagContainer UGA_Leblanc_Ability1::GetPendingCastTags() const
{
	return FGameplayTagContainer(AssassinsGameplayTags::Status_Channeling);
}

void UGA_Leblanc_Ability1::PostPlayMontage()
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

void UGA_Leblanc_Ability1::OnSpawnProjectileNotify(FGameplayEventData Payload)
{
	if (K2_HasAuthority())
	{
		SpawnProjectile();
		return;
	}

	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);
	ServerSetReplicatedEvent(SpawnProjectileEventType);
}

void UGA_Leblanc_Ability1::OnSpawnProjectileRequested()
{
	SpawnProjectile();
}

void UGA_Leblanc_Ability1::SpawnProjectile()
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

void UGA_Leblanc_Ability1::HandleProjectile(AAssassinsProjectile* SpawnedProjectile)
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
