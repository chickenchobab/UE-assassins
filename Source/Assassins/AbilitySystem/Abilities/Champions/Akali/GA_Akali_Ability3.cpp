#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Ability3.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitRecastPress.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"
#include "Character/Champions/Akali/AssassinsShroud.h"
#include "Engine/CurveTable.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

namespace AkaliShurikenFlip
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_FLIP, "Status.Combo.Ability3.1");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_DASH, "Status.Combo.Ability3.2");

	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_SHURIKEN_HIT, "Event.Champion.Akali.ShurikenHit");

	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_TARGET_MARK, "Status.Target.Akali.Ability3");

	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_DASH_MARK_TO_SHROUD, "GameplayCue.Champion.Akali.DashMarkToShroud");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ABILITY4, "Ability.Ability4");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ATTACK, "Event.Attack");
};

UGA_Akali_Ability3::UGA_Akali_Ability3(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DashMarkDurationRow = TEXT("Ability3.DashMarkDuration");
}

void UGA_Akali_Ability3::PrePlayMontage()
{
	Super::PrePlayMontage();

	AddTagToAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_FLIP);

	UAbilityTask_DashTo* FlipTask = UAbilityTask_DashTo::DashTo(this, NAME_None, CalculateFlipLocation(), static_cast<float>(FlipSpeed),
		0.0f, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.0f);
	FlipTask->OnFinished.AddDynamic(this, &ThisClass::OnFlipFinished);
	FlipTask->OnCancelled.AddDynamic(this, &ThisClass::OnFlipFinished);
	FlipTask->ReadyForActivation();
}

FVector UGA_Akali_Ability3::CalculateFlipLocation() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return FVector::ZeroVector;
	}

	return AvatarActor->GetActorLocation() - (AvatarActor->GetActorForwardVector() * FlipDistance);
}

void UGA_Akali_Ability3::OnFlipFinished()
{
	RemoveTagFromAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_FLIP);
}

void UGA_Akali_Ability3::PostPlayMontage()
{
	// 표창이 던지자마자 맞을 수 있어 던지기 전부터 기다린다.
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, AkaliShurikenFlip::TAG_EVENT_SHURIKEN_HIT, nullptr, true, true);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnShurikenHit);
	HitTask->ReadyForActivation();

	if (K2_HasAuthority())
	{
		SpawnProjectile();
	}
}

void UGA_Akali_Ability3::SpawnProjectile()
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	AAssassinsProjectile* Projectile = AvatarActor ? SpawnAbilityProjectile(ProjectileClass, FTransform(FRotator::ZeroRotator, AvatarActor->GetActorLocation())) : nullptr;

	if (Projectile == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	HandleProjectile(Projectile);
}

void UGA_Akali_Ability3::HandleProjectile(AAssassinsProjectile* SpawnedProjectile)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAssassinsChampionSkillState_Akali* AkaliState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Akali>(AvatarActor);
	if (AkaliState == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	AkaliState->SpawnedProjectile = SpawnedProjectile;
	ThrownProjectile = SpawnedProjectile;

	SpawnedProjectile->OnDestroyed.AddDynamic(this, &ThisClass::OnProjectileDestroyed);
	SpawnedProjectile->LaunchStraight(AvatarActor->GetActorForwardVector() * ProjectileSpeed, static_cast<float>(AbilityRange), MakeEffectSpecHandle(ProjectileDamageEffectClass));
}

void UGA_Akali_Ability3::OnProjectileDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != ThrownProjectile)
	{
		return;
	}

	ThrownProjectile = nullptr;

	// 명중 이벤트가 표창이 사라지는 프레임에 함께 올 수 있어, 빗나감은 다음 틱에 판단한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CheckProjectileMissed);
	}
}

void UGA_Akali_Ability3::CheckProjectileMissed()
{
	if (IsActive() && !bProjectileHit)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Akali_Ability3::OnShurikenHit(FGameplayEventData Payload)
{
	bProjectileHit = true;

	AActor* HitActor = const_cast<AActor*>(ToRawPtr(Payload.Target));
	if (const AAssassinsShroud* Shroud = Cast<AAssassinsShroud>(HitActor))
	{
		DashLocationWithoutTarget = Shroud->DashMarkLocation;

		if (K2_HasAuthority())
		{
			FGameplayCueParameters CueParameters;
			CueParameters.Location = DashLocationWithoutTarget;
			K2_AddGameplayCueWithParams(AkaliShurikenFlip::TAG_GAMEPLAYCUE_DASH_MARK_TO_SHROUD, CueParameters, true);

			const float MarkDuration = EvaluateCurveTableRowByAbilityLevel(AbilityMagnitudeTable, DashMarkDurationRow, TEXT("Akali Ability3 dash mark duration"));
			ShroudMarkWaitTask = UAbilityTask_WaitDelay::WaitDelay(this, MarkDuration);
			ShroudMarkWaitTask->OnFinish.AddDynamic(this, &ThisClass::OnDashMarkExpired);
			ShroudMarkWaitTask->ReadyForActivation();
		}
	}
	else
	{
		ProjectileHitTarget = HitActor;

		// 어빌리티는 유닛의 표식이 있는 동안 이어진다. 어빌리티 태스크로 기다려, 어빌리티와 함께 끝나고 다음 활성화를 끝내지 않게 한다.
		if (K2_HasAuthority())
		{
			TargetMarkWaitTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
				this, AkaliShurikenFlip::TAG_STATUS_TARGET_MARK, ProjectileHitTarget, true);
			TargetMarkWaitTask->Removed.AddDynamic(this, &ThisClass::OnTargetMarkRemoved);
			TargetMarkWaitTask->ReadyForActivation();
		}
	}

	WaitForRecast();
}

void UGA_Akali_Ability3::OnDashMarkExpired()
{
	K2_RemoveGameplayCue(AkaliShurikenFlip::TAG_GAMEPLAYCUE_DASH_MARK_TO_SHROUD);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability3::OnTargetMarkRemoved()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability3::WaitForRecast()
{
	// 양쪽 모두 입력을 기다린다. 클라이언트의 입력이 예측 키와 함께 서버에 가서, 양쪽이 같은 예측 창에서 돌진을 시작한다.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Akali_Ability3::OnRecastReleased(float TimeHeld)
{
	// 다른 어빌리티 시전 중 입력은 재시전이 아니다(돌진이 시전을 끊는다). 클라이언트가 받지 않으므로 서버에도 가지 않는다.
	UAbilityTask_WaitRecastPress* PressTask = UAbilityTask_WaitRecastPress::WaitRecastPress(this, FGameplayTagContainer(AssassinsGameplayTags::Status_Channeling));
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastPressed);
	PressTask->ReadyForActivation();
}

void UGA_Akali_Ability3::OnRecastPressed()
{
	EndDashMarkCheckingTask();
	StartDash();
}

void UGA_Akali_Ability3::EndDashMarkCheckingTask()
{
	if (IsValid(TargetMarkWaitTask))
	{
		TargetMarkWaitTask->EndTask();
	}
	TargetMarkWaitTask = nullptr;

	if (IsValid(ShroudMarkWaitTask))
	{
		ShroudMarkWaitTask->EndTask();
	}
	ShroudMarkWaitTask = nullptr;
}

void UGA_Akali_Ability3::StartDash()
{
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DashMontage, 1.0f, NAME_None, true);
	MontageTask->ReadyForActivation();

	AddTagToAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_DASH);

	if (IsValid(ProjectileHitTarget))
	{
		UAbilityTask_DashToActor* DashTask = UAbilityTask_DashToActor::DashToActor(this, NAME_None, ProjectileHitTarget, static_cast<float>(DashSpeed),
			0.0f, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.0f);
		DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashCancelled);
		DashTask->OnFinished.AddDynamic(this, &ThisClass::OnTargetDashFinished);
		DashTask->ReadyForActivation();

		AddCancelledByTag(AkaliShurikenFlip::TAG_ABILITY_ABILITY4);

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ProjectileHitTarget))
		{
			TargetASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AkaliShurikenFlip::TAG_STATUS_TARGET_MARK));
		}
		return;
	}

	if (!DashLocationWithoutTarget.Equals(FVector::ZeroVector, 1.e-4))
	{
		UAbilityTask_DashTo* DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, DashLocationWithoutTarget, static_cast<float>(DashSpeed),
			0.0f, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.0f);
		DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashCancelled);
		DashTask->OnFinished.AddDynamic(this, &ThisClass::OnLocationDashFinished);
		DashTask->ReadyForActivation();

		AddCancelledByTag(AkaliShurikenFlip::TAG_ABILITY_ABILITY4);
		K2_RemoveGameplayCue(AkaliShurikenFlip::TAG_GAMEPLAYCUE_DASH_MARK_TO_SHROUD);
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability3::OnDashCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability3::OnLocationDashFinished()
{
	RemoveTagFromAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_DASH);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability3::OnTargetDashFinished()
{
	RemoveTagFromAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_DASH);

	// 돌진이 자리 잡도록 데미지는 한 프레임 뒤에 준다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ApplyDashDamage);
	}
}

void UGA_Akali_Ability3::ApplyDashDamage()
{
	if (!IsValid(ProjectileHitTarget))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnDashDamageSync);
	SyncTask->ReadyForActivation();
}

void UGA_Akali_Ability3::OnDashDamageSync()
{
	ApplyEffectToTarget(DashDamageEffectClass, ProjectileHitTarget);

	// 서버만 끝낸다. 소유 클라이언트는 돌진이 앞서고 싱크 포인트에서 기다리지 않아 먼저 여기 오는데, 여기서 끝내면 그 종료가
	// 서버의 돌진 중에 도착해 서버의 데미지를 끊는다. 소유 클라이언트는 서버의 종료로 끝난다.
	if (!K2_HasAuthority())
	{
		// 돌진 뒤 공격도 그 종료를 기다린다. 서버는 자기 돌진이 끝날 때까지 공격을 거절하고(Status.Dashing), 공격은 소유 클라이언트만 시작한다.
		bAttackOnEnd = true;
		return;
	}

	// 서버 자신의 플레이어(리슨 서버, 봇)는 여기서, 원격 클라이언트는 이 종료가 도착하면(EndAbility) 공격을 시작한다.
	SendAttackEvent(ProjectileHitTarget);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability3::SendAttackEvent(AActor* AttackTarget)
{
	FGameplayEventData Payload;
	Payload.Target = AttackTarget;
	SendGameplayEvent(AkaliShurikenFlip::TAG_EVENT_ATTACK, Payload);
}

void UGA_Akali_Ability3::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 표창이 어빌리티보다 오래 살아서, 표창의 종료가 다음 활성화에 닿지 않게 푼다.
	if (IsValid(ThrownProjectile))
	{
		ThrownProjectile->OnDestroyed.RemoveDynamic(this, &ThisClass::OnProjectileDestroyed);
	}
	ThrownProjectile = nullptr;

	EndDashMarkCheckingTask();

	// 서버가 돌진 뒤 E를 끝냈을 때만 공격한다. 궁극기 등으로 E가 끊긴 경우는 아니다.
	AActor* AttackTarget = (bAttackOnEnd && !bWasCancelled) ? ProjectileHitTarget.Get() : nullptr;
	bAttackOnEnd = false;

	bProjectileHit = false;
	ProjectileHitTarget = nullptr;
	DashLocationWithoutTarget = FVector::ZeroVector;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// 공격이 E의 종료 안에서 시작하지 않도록 E가 끝난 뒤에 보낸다.
	if (IsValid(AttackTarget))
	{
		SendAttackEvent(AttackTarget);
	}
}
