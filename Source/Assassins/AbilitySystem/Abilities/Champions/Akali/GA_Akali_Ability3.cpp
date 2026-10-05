// Fill out your copyright notice in the Description page of Project Settings.

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
	// Set while Akali flips back, then while she dashes.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_FLIP, "Status.Combo.Ability3.1");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_DASH, "Status.Combo.Ability3.2");

	// Sent by the shuriken, on the server and on the owning client, with what it hit.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_SHURIKEN_HIT, "Event.Champion.Akali.ShurikenHit");

	// The mark the shuriken leaves on a unit.
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
	// Faces the cursor first, so that the flip goes back from there.
	Super::PrePlayMontage();

	AddTagToAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_FLIP);

	UAbilityTask_DashTo* FlipTask = UAbilityTask_DashTo::DashTo(this, NAME_None, CalculateFlipLocation(), static_cast<float>(FlipSpeed),
		/*InAcceptRadius*/ 0.0f, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
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
	// Listened for before the shuriken is thrown: it may hit something right away.
	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, AkaliShurikenFlip::TAG_EVENT_SHURIKEN_HIT, /*OptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnShurikenHit);
	HitTask->ReadyForActivation();

	// Only the server throws. The shuriken replicates, and tells the owning client what it hit.
	if (K2_HasAuthority())
	{
		SpawnProjectile();
	}
}

void UGA_Akali_Ability3::SpawnProjectile()
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	// Thrown from where Akali stands. The velocity gives the direction.
	AAssassinsProjectile* Projectile = AvatarActor ? SpawnAbilityProjectile(ProjectileClass, FTransform(FRotator::ZeroRotator, AvatarActor->GetActorLocation())) : nullptr;

	// Without a shuriken nothing would ever end the ability.
	if (Projectile == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
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
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	// The shroud finds the shuriken through Akali's skill state.
	AkaliState->SpawnedProjectile = SpawnedProjectile;
	ThrownProjectile = SpawnedProjectile;

	SpawnedProjectile->OnDestroyed.AddDynamic(this, &ThisClass::OnProjectileDestroyed);
	SpawnedProjectile->LaunchStraight(AvatarActor->GetActorForwardVector() * ProjectileSpeed, static_cast<float>(AbilityRange), MakeEffectSpecHandle(ProjectileDamageEffectClass));
}

void UGA_Akali_Ability3::OnProjectileDestroyed(AActor* DestroyedActor)
{
	// Only this activation's shuriken counts.
	if (DestroyedActor != ThrownProjectile)
	{
		return;
	}

	ThrownProjectile = nullptr;

	// The hit event may come in the same frame as the shuriken goes away.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CheckProjectileMissed);
	}
}

void UGA_Akali_Ability3::CheckProjectileMissed()
{
	// The shuriken went away without hitting anything: there is nothing to dash to.
	if (IsActive() && !bProjectileHit)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
	}
}

void UGA_Akali_Ability3::OnShurikenHit(FGameplayEventData Payload)
{
	bProjectileHit = true;

	AActor* HitActor = const_cast<AActor*>(ToRawPtr(Payload.Target));
	if (const AAssassinsShroud* Shroud = Cast<AAssassinsShroud>(HitActor))
	{
		// The shuriken marked the shroud: Akali can dash there for a while, even once the shroud is gone.
		DashLocationWithoutTarget = Shroud->DashMarkLocation;

		if (K2_HasAuthority())
		{
			FGameplayCueParameters CueParameters;
			CueParameters.Location = DashLocationWithoutTarget;
			K2_AddGameplayCueWithParams(AkaliShurikenFlip::TAG_GAMEPLAYCUE_DASH_MARK_TO_SHROUD, CueParameters, /*bRemoveOnAbilityEnd*/ true);

			const float MarkDuration = EvaluateCurveTableRowByAbilityLevel(AbilityMagnitudeTable, DashMarkDurationRow, TEXT("Akali Ability3 dash mark duration"));
			ShroudMarkWaitTask = UAbilityTask_WaitDelay::WaitDelay(this, MarkDuration);
			ShroudMarkWaitTask->OnFinish.AddDynamic(this, &ThisClass::OnDashMarkExpired);
			ShroudMarkWaitTask->ReadyForActivation();
		}
	}
	else
	{
		ProjectileHitTarget = HitActor;

		// The ability lasts as long as the mark on the unit. An ability task, so that it ends with the ability and
		// cannot end a later activation.
		if (K2_HasAuthority())
		{
			TargetMarkWaitTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
				this, AkaliShurikenFlip::TAG_STATUS_TARGET_MARK, ProjectileHitTarget, /*OnlyTriggerOnce*/ true);
			TargetMarkWaitTask->Removed.AddDynamic(this, &ThisClass::OnTargetMarkRemoved);
			TargetMarkWaitTask->ReadyForActivation();
		}
	}

	WaitForRecast();
}

void UGA_Akali_Ability3::OnDashMarkExpired()
{
	K2_RemoveGameplayCue(AkaliShurikenFlip::TAG_GAMEPLAYCUE_DASH_MARK_TO_SHROUD);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability3::OnTargetMarkRemoved()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability3::WaitForRecast()
{
	// Both sides wait for the input: the client's press reaches the server with its prediction key, so both start the
	// dash in the same prediction window.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/ true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Akali_Ability3::OnRecastReleased(float TimeHeld)
{
	// A press during the cast of another ability is no recast: the dash would cut the cast short. The client does not
	// take it, so the server never hears of it either.
	UAbilityTask_WaitRecastPress* PressTask = UAbilityTask_WaitRecastPress::WaitRecastPress(this, FGameplayTagContainer(AssassinsGameplayTags::Status_Channeling));
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastPressed);
	PressTask->ReadyForActivation();
}

void UGA_Akali_Ability3::OnRecastPressed()
{
	// The dash spends the mark: its end must not end the dash.
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
		this, NAME_None, DashMontage, /*Rate*/ 1.0f, NAME_None, /*bStopWhenAbilityEnds*/ true);
	MontageTask->ReadyForActivation();

	AddTagToAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_DASH);

	if (IsValid(ProjectileHitTarget))
	{
		UAbilityTask_DashToActor* DashTask = UAbilityTask_DashToActor::DashToActor(this, NAME_None, ProjectileHitTarget, static_cast<float>(DashSpeed),
			/*InAcceptRadius*/ 0.0f, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
		DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashCancelled);
		DashTask->OnFinished.AddDynamic(this, &ThisClass::OnTargetDashFinished);
		DashTask->ReadyForActivation();

		// Once Akali dashes, her ultimate may cut in.
		AddCancelledByTag(AkaliShurikenFlip::TAG_ABILITY_ABILITY4);

		// The mark is spent.
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ProjectileHitTarget))
		{
			TargetASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(AkaliShurikenFlip::TAG_STATUS_TARGET_MARK));
		}
		return;
	}

	if (!DashLocationWithoutTarget.Equals(FVector::ZeroVector, 1.e-4))
	{
		UAbilityTask_DashTo* DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, DashLocationWithoutTarget, static_cast<float>(DashSpeed),
			/*InAcceptRadius*/ 0.0f, ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
		DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashCancelled);
		DashTask->OnFinished.AddDynamic(this, &ThisClass::OnLocationDashFinished);
		DashTask->ReadyForActivation();

		AddCancelledByTag(AkaliShurikenFlip::TAG_ABILITY_ABILITY4);
		K2_RemoveGameplayCue(AkaliShurikenFlip::TAG_GAMEPLAYCUE_DASH_MARK_TO_SHROUD);
		return;
	}

	// The shuriken hit a unit that is gone by now.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability3::OnDashCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability3::OnLocationDashFinished()
{
	RemoveTagFromAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_DASH);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability3::OnTargetDashFinished()
{
	RemoveTagFromAvatar(AkaliShurikenFlip::TAG_STATUS_COMBO_DASH);

	// The damage waits a frame, for the dash to have settled.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ApplyDashDamage);
	}
}

void UGA_Akali_Ability3::ApplyDashDamage()
{
	// The target may have died during the dash: the ability still has to end.
	if (!IsValid(ProjectileHitTarget))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	// The only sync point of the activation: the server waits for the client to arrive as well.
	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnDashDamageSync);
	SyncTask->ReadyForActivation();
}

void UGA_Akali_Ability3::OnDashDamageSync()
{
	ApplyEffectToTarget(DashDamageEffectClass, ProjectileHitTarget);

	// Only the server ends. The owning client gets here first, as its dash is ahead and it does not wait at the sync
	// point: its end would reach the server while the server's dash is still going, and cut the server's damage off. The
	// owning client ends with the server's end.
	if (!K2_HasAuthority())
	{
		// The attack after the dash waits for that end too: until its own dash is over the server turns the attack down
		// (Status.Dashing), and the attack is one only the owning client starts.
		bAttackOnEnd = true;
		return;
	}

	// Akali goes on attacking the target: the server's own player(a listen server's, a bot) starts the attack here, a
	// remote client once this end reaches it(EndAbility).
	SendAttackEvent(ProjectileHitTarget);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability3::SendAttackEvent(AActor* AttackTarget)
{
	FGameplayEventData Payload;
	Payload.Target = AttackTarget;
	SendGameplayEvent(AkaliShurikenFlip::TAG_EVENT_ATTACK, Payload);
}

void UGA_Akali_Ability3::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// The shuriken outlives the ability: its end must not reach the next activation.
	if (IsValid(ThrownProjectile))
	{
		ThrownProjectile->OnDestroyed.RemoveDynamic(this, &ThisClass::OnProjectileDestroyed);
	}
	ThrownProjectile = nullptr;

	EndDashMarkCheckingTask();

	// The owning client attacks once the server ended E after its dash, not when E was cut short(e.g. by her ultimate).
	AActor* AttackTarget = (bAttackOnEnd && !bWasCancelled) ? ProjectileHitTarget.Get() : nullptr;
	bAttackOnEnd = false;

	bProjectileHit = false;
	ProjectileHitTarget = nullptr;
	DashLocationWithoutTarget = FVector::ZeroVector;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// After E is over, so that the attack does not start inside its end.
	if (IsValid(AttackTarget))
	{
		SendAttackEvent(AttackTarget);
	}
}
