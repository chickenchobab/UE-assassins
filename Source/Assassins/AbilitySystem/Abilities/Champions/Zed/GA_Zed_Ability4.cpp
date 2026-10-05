#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_Ability4.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsCharacter.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameplayCueFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "NativeGameplayTags.h"
#include "Player/AssassinsPlayerController.h"
#include "TimerManager.h"

namespace ZedUltimate
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY4, "Status.Combo.Ability4");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_ULTIMATE_CAST, "GameplayCue.Champion.Zed.UltimateCast");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_ULTIMATE_CAST_TARGET, "GameplayCue.Champion.Zed.UltimateCastTarget");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SETBYCALLER_STORED_DAMAGE, "SetByCaller.Zed.StoredDamage");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ATTACK, "Event.Attack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ATTACK, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ACTIVATE_ATTACK, "Ability.ActivateAttack");

	static constexpr float EffectEventDelay = 0.4f;

	static constexpr double BehindTargetDistance = 150.0;
};

UGA_Zed_Ability4::UGA_Zed_Ability4(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Zed_Ability4::PrePlayMontage()
{
	// Super를 부르지 않는다. 궁극기는 대상으로 돌거나 몽타주를 고르지 않고 항상 MontageToPlay를 재생한다.

	AddTagToAvatar(AssassinsGameplayTags::Status_Channeling);
	if (AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		AvatarCharacter->OnChannelingEnded.AddDynamic(this, &ThisClass::AbortPausedMovement);
	}

	K2_ExecuteGameplayCue(ZedUltimate::TAG_GAMEPLAYCUE_ULTIMATE_CAST, FGameplayEffectContextHandle());
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(EffectEventTimerHandle, this, &ThisClass::EffectEvent, ZedUltimate::EffectEventDelay, false);
	}

	SpawnShadow();

	SetCapsuleCollisionEnabled(false);
	if (IsLocallyControlled())
	{
		AddTagToAvatar(ZedUltimate::TAG_STATUS_COMBO_ABILITY4);
	}
}

FGameplayTagContainer UGA_Zed_Ability4::GetPendingCastTags() const
{
	return FGameplayTagContainer(AssassinsGameplayTags::Status_Channeling);
}

void UGA_Zed_Ability4::SpawnShadow()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return;
	}

	const FTransform SpawnTransform(FRotator(0.0, AvatarActor->GetActorRotation().Yaw, 0.0), AvatarActor->GetActorLocation());
	AAssassinsZedShadow* Shadow = AAssassinsZedShadow::SpawnShadow(ShadowClass, AvatarActor, SpawnTransform, ShadowLifeSpan);
	if (Shadow == nullptr)
	{
		return;
	}

	SpawnedShadow = Shadow;
	Shadow->OnEndPlay.AddDynamic(this, &ThisClass::OnShadowEndPlay);
}

void UGA_Zed_Ability4::EffectEvent()
{
	if (IsValid(AbilityTargetActor))
	{
		UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(AbilityTargetActor, ZedUltimate::TAG_GAMEPLAYCUE_ULTIMATE_CAST_TARGET, FGameplayCueParameters());
	}
}

void UGA_Zed_Ability4::OnMontageComplete()
{
	// Super를 부르지 않는다. Super는 어빌리티를 끝내는데, 시전은 절반일 뿐이다.

	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);
	SetCapsuleCollisionEnabled(true);

	if (!IsValid(AbilityTargetActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 조종하는 쪽이 제드를 옮긴다. 순간이동은 예측되어 이동과 함께 서버에 간다.
	if (IsLocallyControlled())
	{
		RemoveTagFromAvatar(ZedUltimate::TAG_STATUS_COMBO_ABILITY4);
		TeleportToTarget();
	}

	UAbilityTask_PlayMontageAndWait* StrikeTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, StrikeMontage, 1.0f, NAME_None, true);
	StrikeTask->OnCompleted.AddDynamic(this, &ThisClass::OnStrikeCompleted);
	StrikeTask->ReadyForActivation();

	StartMark();

	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Zed_Ability4::OnMontageCanceled()
{
	// 시전이 끊기면 궁극기를 끝낸다. 그림자가 사라질 때까지 제드가 월드 밖에 멈춰 있으면 안 된다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Zed_Ability4::StartMark()
{
	if (UAssassinsHealthComponent* TargetHealth = UAssassinsHealthComponent::FindHealthComponent(AbilityTargetActor))
	{
		TargetHealth->OnHealthChanged.AddDynamic(this, &ThisClass::StoreDamage);
		MarkedHealthComponent = TargetHealth;
	}

	bDetonationPending = true;

	UWorld* World = GetWorld();
	if ((World != nullptr) && (DamageEffectDelay > 0.0))
	{
		World->GetTimerManager().SetTimer(DetonationTimerHandle, this, &ThisClass::DeathMarkDetonation, static_cast<float>(DamageEffectDelay), false);
	}
	else
	{
		DeathMarkDetonation();
	}
}

void UGA_Zed_Ability4::StoreDamage(UAssassinsHealthComponent* ChangedHealthComponent, float OldValue, float NewValue, AActor* DamageInstigator)
{
	const double Damage = OldValue - NewValue;
	if ((Damage > 0.0) && (DamageInstigator == GetAvatarActorFromActorInfo()))
	{
		StoredDamage += Damage;
	}
}

void UGA_Zed_Ability4::DeathMarkDetonation()
{
	UnbindMarkedHealth();

	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnDetonationSync);
	SyncTask->ReadyForActivation();
}

void UGA_Zed_Ability4::OnDetonationSync()
{
	DamageSpecHandle = MakeEffectSpecHandle(DamageEffectClass);
	if (DamageSpecHandle.IsValid())
	{
		DamageSpecHandle.Data->SetSetByCallerMagnitude(ZedUltimate::TAG_SETBYCALLER_STORED_DAMAGE, static_cast<float>(StoredDamage));
		ApplyGameplayEffectSpecToTargetActor(DamageSpecHandle, AbilityTargetActor);
	}

	IsDamageDone = true;
	bDetonationPending = false;

	EndIfDone();
}

void UGA_Zed_Ability4::OnStrikeCompleted()
{
	if (IsLocallyControlled() && IsValid(AbilityTargetActor))
	{
		FGameplayEventData Payload;
		Payload.Target = AbilityTargetActor;
		SendGameplayEvent(ZedUltimate::TAG_EVENT_ATTACK, Payload);
	}
}

void UGA_Zed_Ability4::OnRecastReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, true);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastPressed);
	PressTask->ReadyForActivation();
}

void UGA_Zed_Ability4::OnRecastPressed(float TimeWaited)
{
	if (!IsValid(SpawnedShadow))
	{
		HasMovedToShadow = true;
		EndIfDone();
		return;
	}

	MoveToShadow();
	HasMovedToShadow = true;

	EndIfDone();
}

void UGA_Zed_Ability4::EndIfDone()
{
	// 소유 클라이언트는 서버를 기다리지 않고 자기 타이머로 터뜨린다(OnlyServerWait). 제드가 돌아간 뒤 거기서 끝내면
	// 그 종료가 서버의 폭발보다 먼저 도착해 끊는다.
	if (!K2_HasAuthority())
	{
		return;
	}

	if (HasMovedToShadow && !bDetonationPending)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Zed_Ability4::OnShadowEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (Actor != SpawnedShadow)
	{
		return;
	}

	UnbindShadow();
	SpawnedShadow = nullptr;

	HasMovedToShadow = true;
	EndIfDone();
}

void UGA_Zed_Ability4::MoveToShadow()
{
	if (!IsValid(SpawnedShadow))
	{
		return;
	}

	SpawnedShadow->SwapPlacesWith(GetAssassinsCharacterFromActorInfo());

	if (UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponentFromActorInfo())
	{
		AssassinsASC->K2_CancelAbilities(ZedUltimate::TAG_ABILITY_ATTACK, FGameplayTag());
		AssassinsASC->K2_CancelAbilities(ZedUltimate::TAG_ABILITY_ACTIVATE_ATTACK, FGameplayTag());
	}
}

void UGA_Zed_Ability4::TeleportToTarget()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if ((AvatarActor == nullptr) || !IsValid(AbilityTargetActor))
	{
		return;
	}

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = AbilityTargetActor->GetActorLocation();
	const FRotator ToTarget = UKismetMathLibrary::FindLookAtRotation(FVector(AvatarLocation.X, AvatarLocation.Y, 0.0), FVector(TargetLocation.X, TargetLocation.Y, 0.0));
	const double TeleportDistance = FVector::Dist2D(AvatarLocation, TargetLocation) + ZedUltimate::BehindTargetDistance;
	const FVector TeleportLocation = AvatarLocation + (ToTarget.Vector() * TeleportDistance);

	const FRotator TeleportRotation = UKismetMathLibrary::FindLookAtRotation(FVector(TeleportLocation.X, TeleportLocation.Y, 0.0), FVector(TargetLocation.X, TargetLocation.Y, 0.0));

	SetAvatarLocationAndRotation(TeleportLocation, TeleportRotation);
}

void UGA_Zed_Ability4::SetCapsuleCollisionEnabled(bool bCollisionEnabled)
{
	if (const ACharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		if (UCapsuleComponent* Capsule = AvatarCharacter->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(bCollisionEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		}
	}
}

void UGA_Zed_Ability4::AbortPausedMovement()
{
	UnbindChannelingEnded();

	if (AAssassinsPlayerController* AssassinsPC = GetAssassinsPlayerControllerFromActorInfo())
	{
		AssassinsPC->StopMovement();
	}
}

void UGA_Zed_Ability4::UnbindChannelingEnded()
{
	if (AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		AvatarCharacter->OnChannelingEnded.RemoveDynamic(this, &ThisClass::AbortPausedMovement);
	}
}

void UGA_Zed_Ability4::UnbindMarkedHealth()
{
	if (UAssassinsHealthComponent* TargetHealth = MarkedHealthComponent.Get())
	{
		TargetHealth->OnHealthChanged.RemoveDynamic(this, &ThisClass::StoreDamage);
	}
	MarkedHealthComponent.Reset();
}

void UGA_Zed_Ability4::UnbindShadow()
{
	if (IsValid(SpawnedShadow))
	{
		SpawnedShadow->OnEndPlay.RemoveDynamic(this, &ThisClass::OnShadowEndPlay);
	}
}

void UGA_Zed_Ability4::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UnbindChannelingEnded();
	UnbindMarkedHealth();
	UnbindShadow();
	SpawnedShadow = nullptr;

	// 어빌리티 객체는 다음 활성화에 다시 쓰이므로, 남은 타이머가 그 안에서 울린다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EffectEventTimerHandle);
		World->GetTimerManager().ClearTimer(DetonationTimerHandle);
	}

	SetCapsuleCollisionEnabled(true);

	StoredDamage = 0.0;
	HasMovedToShadow = false;
	IsDamageDone = false;
	bDetonationPending = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
