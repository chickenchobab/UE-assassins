// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Ability4.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"
#include "Components/CapsuleComponent.h"
#include "NativeGameplayTags.h"
#include "Player/AssassinsPlayerController.h"

namespace AkaliUltimate
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ULTIMATE_RECAST, "Event.Champion.Akali.UltimateRecast");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY4_1, "Status.Combo.Ability4.1");

	// The server tells the owning client the dash hit the target. GameCustom1 is the arrival of UGA_UnitTargeted.
	static constexpr EAbilityCustomReplicatedEvent HitNotifyEvent = EAbilityCustomReplicatedEvent::GameCustom2;

	// The owning client tells the server it saw the dash reach the target.
	static constexpr EAbilityCustomReplicatedEvent HitPredictionEvent = EAbilityCustomReplicatedEvent::GameCustom3;

	// How far the target may be off the way of the dash, beyond the reach of the capsule, for the server to take the hit
	// the owning client saw: about what a target walks while the news of the hit travels, not a flash away.
	static constexpr double HitPredictionTolerance = 150.0;

	// Directions are taken on the ground, like Vector_Normal2D with its tolerance.
	static constexpr double DirectionTolerance = 1.e-4;
};

UGA_Akali_Ability4::UGA_Akali_Ability4(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Akali_Ability4::PrePlayMontage()
{
	// Picks the montage, faces the target, and ends the ability when the target is gone.
	Super::PrePlayMontage();

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsActive() || (AvatarActor == nullptr))
	{
		return;
	}

	DashStartLocation = AvatarActor->GetActorLocation();
	const FVector StartToTarget = AbilityTargetActor->GetActorLocation() - DashStartLocation;
	ToTarget = FVector(StartToTarget.X, StartToTarget.Y, 0.0);

	// Until the dash hits the target.
	AddTagToAvatar(AkaliUltimate::TAG_STATUS_COMBO_ABILITY4_1);

	// Through the target, DashDistance past it.
	const FVector DashLocation = DashStartLocation + (ToTarget.GetSafeNormal2D(AkaliUltimate::DirectionTolerance) * (DashDistance + ToTarget.Size2D()));

	UAbilityTask_DashTo* DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, DashLocation, DashSpeed, /*InAcceptRadius*/ 0.0f,
		ERootMotionFinishVelocityMode::ClampVelocity, /*SetVelocityOnFinish*/ FVector::ZeroVector, /*ClampVelocityOnFinish*/ 0.0f);
	DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->ReadyForActivation();
}

FGameplayTagContainer UGA_Akali_Ability4::GetPendingCastTags() const
{
	// What the dash holds until it hits the target(PrePlayMontage): her E and W wait for the hit, before the dash too.
	// And what keeps her Q and the move click off in place of Status.Dashing, which the dash sets rather than adds, so it
	// can't be held beforehand: without it a Q pressed meanwhile starts, and the dash cuts it short.
	FGameplayTagContainer PendingCastTags(AkaliUltimate::TAG_STATUS_COMBO_ABILITY4_1);
	PendingCastTags.AddTag(AssassinsGameplayTags::Status_Channeling);
	return PendingCastTags;
}

void UGA_Akali_Ability4::PostPlayMontage()
{
	// After the montage of the dash has started, so that the montage of the hit plays over it, even for a target hit
	// right away. Enemies overlapping already count, the target included.
	AcquireCapsule();

	if (K2_HasAuthority())
	{
		if (!IsLocallyControlled())
		{
			UAbilityTask_WaitReplicatedEvent* HitPredictionTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, AkaliUltimate::HitPredictionEvent);
			HitPredictionTask->OnEvent.AddDynamic(this, &ThisClass::OnClientPredictedHit);
			HitPredictionTask->ReadyForActivation();
		}
		return;
	}

	// For a hit the owning client's own capsule missed.
	UAbilityTask_WaitReplicatedEvent* HitNotifyTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, AkaliUltimate::HitNotifyEvent);
	HitNotifyTask->OnEvent.AddDynamic(this, &ThisClass::ClientGetNotifiedOfHitEvent);
	HitNotifyTask->ReadyForActivation();
}

void UGA_Akali_Ability4::AcquireCapsule()
{
	UAssassinsChampionSkillState_Akali* AkaliState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Akali>(GetAvatarActorFromActorInfo());
	if (AkaliState == nullptr)
	{
		return;
	}

	AbilityCapsule = AkaliState->AcquireUltimateCapsule(this);
	if (AbilityCapsule == nullptr)
	{
		return;
	}

	CapsuleRadius = AbilityCapsule->GetScaledCapsuleRadius();
	AbilityCapsule->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnTargetHit);

	TArray<AActor*> OverlappingActors;
	AbilityCapsule->GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		HandleOverlappedActor(OverlappingActor);
	}
}

void UGA_Akali_Ability4::ReleaseCapsule()
{
	if (AbilityCapsule == nullptr)
	{
		return;
	}

	AbilityCapsule->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnTargetHit);
	AbilityCapsule = nullptr;

	// The collision stays on while the recast holds the capsule.
	if (UAssassinsChampionSkillState_Akali* AkaliState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Akali>(GetAvatarActorFromActorInfo()))
	{
		AkaliState->ReleaseUltimateCapsule(this);
	}
}

void UGA_Akali_Ability4::OnTargetHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HandleOverlappedActor(OtherActor);
}

void UGA_Akali_Ability4::HandleOverlappedActor(AActor* TargetActor)
{
	if (!IsActive() || !IsValidEnemy(TargetActor) || OverlappedActors.Contains(TargetActor))
	{
		return;
	}

	// The owning client only sees the hit of the target coming. The hits are the server's.
	if (!K2_HasAuthority())
	{
		if (TargetActor == AbilityTargetActor)
		{
			PredictTargetHit();
		}
		return;
	}

	OverlappedActors.Add(TargetActor);
	ApplyGameplayEffectSpecToTargetActor(MakeEffectSpecHandle(DamageEffectClass), TargetActor);

	HasHitAbilityTarget = HasHitAbilityTarget || (TargetActor == AbilityTargetActor);
	if (!HasHitAbilityTarget || HasServerNotifiedClientHitEvent)
	{
		return;
	}

	HasServerNotifiedClientHitEvent = true;
	if (!IsLocallyControlled())
	{
		ClientSetReplicatedEvent(AkaliUltimate::HitNotifyEvent);
	}

	PlayHitDash();
}

void UGA_Akali_Ability4::PredictTargetHit()
{
	if (HasHitAbilityTarget)
	{
		return;
	}

	HasHitAbilityTarget = true;

	// Without waiting a round trip for the server, by which time the dash has carried Akali through the target: E right
	// after the hit flips her back toward her start, with lag too. The server takes the hit as it hears of it, before
	// anything the hit lets the client cast(her E or W), which it would turn down while its own dash has not reached the
	// target yet.
	SendPredictedEventToServer(AkaliUltimate::HitPredictionEvent, [this]() { PlayHitDash(); });
}

void UGA_Akali_Ability4::ClientGetNotifiedOfHitEvent()
{
	// The owning client may have seen the hit already.
	if (HasHitAbilityTarget)
	{
		return;
	}

	HasHitAbilityTarget = true;
	PlayHitDash();
}

void UGA_Akali_Ability4::OnClientPredictedHit()
{
	// The server saw the hit first, or the target got away from where the dash goes: the server's own hit decides.
	if (HasHitAbilityTarget || !IsTargetAlongDash())
	{
		return;
	}

	HandleOverlappedActor(AbilityTargetActor);
}

bool UGA_Akali_Ability4::IsTargetAlongDash() const
{
	if (!IsValid(AbilityTargetActor))
	{
		return false;
	}

	// On the ground, as the dash goes: from its start through where the target stood, DashDistance past it.
	const FVector DashDirection = ToTarget.GetSafeNormal2D(AkaliUltimate::DirectionTolerance);
	const FVector DashStart(DashStartLocation.X, DashStartLocation.Y, 0.0);
	const FVector DashEnd = DashStart + (DashDirection * (DashDistance + ToTarget.Size2D()));

	const FVector TargetLocation = AbilityTargetActor->GetActorLocation();
	const double TargetDistance = FMath::PointDistToSegment(FVector(TargetLocation.X, TargetLocation.Y, 0.0), DashStart, DashEnd);
	return TargetDistance <= (CapsuleRadius + AbilityTargetActor->GetSimpleCollisionRadius() + AkaliUltimate::HitPredictionTolerance);
}

void UGA_Akali_Ability4::PlayHitDash()
{
	RemoveTagFromAvatar(AkaliUltimate::TAG_STATUS_COMBO_ABILITY4_1);

	// The owning client plays the hit in the window of the sync point, and the server when the client did.
	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnHitDashSynced);
	SyncTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnHitDashSynced()
{
	UAbilityTask_PlayMontageAndWait* HitMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, HitDashMontage, /*Rate*/ 1.0f, NAME_None, /*bStopWhenAbilityEnds*/ true);
	HitMontageTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnDashEnded()
{
	// The capsule only hits during the dash.
	ReleaseCapsule();

	if (IsLocallyControlled())
	{
		WaitAndHandleRecast();
	}
}

void UGA_Akali_Ability4::WaitAndHandleRecast()
{
	UAbilityTask_WaitDelay* PermissionTask = UAbilityTask_WaitDelay::WaitDelay(this, RecastPermissionTime);
	PermissionTask->OnFinish.AddDynamic(this, &ThisClass::OnRecastPermissionEnded);
	PermissionTask->ReadyForActivation();

	UAbilityTask_WaitDelay* RecastDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, RecastDelay);
	RecastDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnRecastAllowed);
	RecastDelayTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnRecastPermissionEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability4::OnRecastAllowed()
{
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/ true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastInputReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnRecastInputReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, /*bTestAlreadyPressed*/ false);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastInputPressed);
	PressTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnRecastInputPressed(float TimeWaited)
{
	// Pressed while another ability is cast(Status.Channeling), the recast would cut it short: it cancels Q. The press
	// is turned down, as a press of another ability during a cast is, and the recast waits for the next one. The owning
	// client alone decides: the server only sees the recast it sends.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(AssassinsGameplayTags::Status_Channeling))
	{
		OnRecastAllowed();
		return;
	}

	Recast();
}

void UGA_Akali_Ability4::Recast()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC == nullptr)
	{
		return;
	}

	// The recast is an ability of its own, triggered by this event. It reads where it goes out of the context, on the
	// server too: the event data goes with its activation.
	RecastEventContext = ASC->MakeEffectContext();
	if (AAssassinsPlayerController* AssassinsPlayerController = GetAssassinsPlayerControllerFromActorInfo())
	{
		FHitResult CursorHitResult;
#if !UE_BUILD_SHIPPING
		// The automation aims in place of the mouse.
		AActor* AimOverrideTarget = nullptr;
		if (!AssassinsPlayerController->GetAimOverride(CursorHitResult, AimOverrideTarget))
#endif
		AssassinsPlayerController->GetHitResultUnderCursorByChannel(TraceTypeQuery1, /*bTraceComplex*/ true, CursorHitResult);
		RecastEventContext.AddHitResult(CursorHitResult, /*bReset*/ false);
	}

	FGameplayEventData Payload;
	Payload.ContextHandle = RecastEventContext;
	SendGameplayEvent(AkaliUltimate::TAG_EVENT_ULTIMATE_RECAST, Payload);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability4::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ReleaseCapsule();

	// Reset on both sides: the client's flags would otherwise carry over to the next activation.
	OverlappedActors.Empty();
	HasHitAbilityTarget = false;
	HasServerNotifiedClientHitEvent = false;
	RecastEventContext = FGameplayEffectContextHandle();
	CapsuleRadius = 0.0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
