#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_Ability1.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsCharacter.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NativeGameplayTags.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

namespace ZedRazorShuriken
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY1, "Status.Combo.Ability1");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY3, "Status.Combo.Ability3");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ANIM_NOTIFY_STATE_END, "Event.AnimNotifyState.End");

	static const FVector TrailOffset(70.0, 0.0, 0.0);
	static constexpr float TrailDuration = 0.3f;
};

UGA_Zed_Ability1::UGA_Zed_Ability1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayTag UGA_Zed_Ability1::GetComboTag() const
{
	return ZedRazorShuriken::TAG_STATUS_COMBO_ABILITY1;
}

FGameplayTag UGA_Zed_Ability1::GetPartnerComboTag() const
{
	return ZedRazorShuriken::TAG_STATUS_COMBO_ABILITY3;
}

void UGA_Zed_Ability1::PrePlayMontage()
{
	if (AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		AvatarCharacter->FreezeRotation();
	}
	AddTagToAvatar(AssassinsGameplayTags::Status_Channeling);

	Super::PrePlayMontage();

	BeginCombo();
	SpawnTrail();
}

void UGA_Zed_Ability1::SpawnTrail()
{
	const AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if ((AvatarCharacter == nullptr) || (TrailParticle == nullptr))
	{
		return;
	}

	SpawnedParticleSystem = UGameplayStatics::SpawnEmitterAttached(TrailParticle, AvatarCharacter->GetRootComponent(), NAME_None,
		ZedRazorShuriken::TrailOffset, FRotator::ZeroRotator, FVector::OneVector, EAttachLocation::KeepRelativeOffset, true);

	UAbilityTask_WaitDelay* TrailTask = UAbilityTask_WaitDelay::WaitDelay(this, ZedRazorShuriken::TrailDuration);
	TrailTask->OnFinish.AddDynamic(this, &ThisClass::OnTrailExpired);
	TrailTask->ReadyForActivation();
}

void UGA_Zed_Ability1::OnTrailExpired()
{
	if (IsValid(SpawnedParticleSystem))
	{
		SpawnedParticleSystem->Deactivate();
	}
}

void UGA_Zed_Ability1::PostPlayMontage()
{
	if (IsLocallyControlled())
	{
		UAbilityTask_WaitGameplayEvent* ThrowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, ZedRazorShuriken::TAG_EVENT_ANIM_NOTIFY_STATE_END, nullptr, true, true);
		ThrowTask->EventReceived.AddDynamic(this, &ThisClass::OnThrowEnded);
		ThrowTask->ReadyForActivation();
	}

	if (K2_HasAuthority())
	{
		ThrowFrom(GetAvatarActorFromActorInfo());
	}

	WaitToImitate();
}

void UGA_Zed_Ability1::OnThrowEnded(FGameplayEventData Payload)
{
	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);
}

void UGA_Zed_Ability1::ImitateWith(AAssassinsZedShadow& Shadow)
{
	const FVector ShadowLocation = Shadow.GetActorLocation();
	Shadow.SetActorRotation(UKismetMathLibrary::FindLookAtRotation(FVector(ShadowLocation.X, ShadowLocation.Y, 0.0), FVector(CursorLocation.X, CursorLocation.Y, 0.0)), ETeleportType::TeleportPhysics);
	Shadow.CallRazorShuriken();

	if (K2_HasAuthority())
	{
		ThrowFrom(&Shadow);
	}
}

void UGA_Zed_Ability1::ThrowFrom(AActor* SourceActor)
{
	if (!IsValid(SourceActor))
	{
		return;
	}

	const FVector SourceLocation = SourceActor->GetActorLocation();
	const FTransform SpawnTransform(SourceActor->GetActorRotation(), FVector(SourceLocation.X, SourceLocation.Y, 0.0));
	AAssassinsProjectile* Projectile = SpawnAbilityProjectile(ProjectileClass, SpawnTransform, SourceActor);
	if (Projectile == nullptr)
	{
		return;
	}

	Projectile->DamageSpecHandle_FirstHit = MakeEffectSpecHandle(FirstHitDamageEffectClass);
	Projectile->LaunchStraight(SourceActor->GetActorForwardVector() * ProjectileSpeed, static_cast<float>(AbilityRange), MakeEffectSpecHandle(DamageEffectClass));
}

void UGA_Zed_Ability1::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsValid(SpawnedParticleSystem))
	{
		SpawnedParticleSystem->Deactivate();
	}
	SpawnedParticleSystem = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
