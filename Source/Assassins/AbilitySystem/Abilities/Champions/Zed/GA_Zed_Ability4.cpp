// Fill out your copyright notice in the Description page of Project Settings.

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

	// The cue on the target plays a moment after the cast starts.
	static constexpr float EffectEventDelay = 0.4f;

	// How far behind the target Zed reappears.
	static constexpr double BehindTargetDistance = 150.0;
};

UGA_Zed_Ability4::UGA_Zed_Ability4(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Zed_Ability4::PrePlayMontage()
{
	// Not calling Super: the ultimate neither turns to the target nor picks a montage, it always plays MontageToPlay.

	// Zed stays still for the cast, and stays still once it is over.
	AddTagToAvatar(AssassinsGameplayTags::Status_Channeling);
	if (AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		AvatarCharacter->OnChannelingEnded.AddDynamic(this, &ThisClass::AbortPausedMovement);
	}

	K2_ExecuteGameplayCue(ZedUltimate::TAG_GAMEPLAYCUE_ULTIMATE_CAST, FGameplayEffectContextHandle());
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(EffectEventTimerHandle, this, &ThisClass::EffectEvent, ZedUltimate::EffectEventDelay, /*bLoop*/ false);
	}

	SpawnShadow();

	// Zed is out of the world while he casts.
	SetCapsuleCollisionEnabled(false);
	if (IsLocallyControlled())
	{
		AddTagToAvatar(ZedUltimate::TAG_STATUS_COMBO_ABILITY4);
	}
}

FGameplayTagContainer UGA_Zed_Ability4::GetPendingCastTags() const
{
	// What the cast holds(PrePlayMontage).
	return FGameplayTagContainer(AssassinsGameplayTags::Status_Channeling);
}

void UGA_Zed_Ability4::SpawnShadow()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return;
	}

	// The shadow stays where Zed stood, facing where he faced.
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
	// Not calling Super, which ends the ability: the cast is only the first half of it.

	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);
	SetCapsuleCollisionEnabled(true);

	// The mark needs its target: there is nothing to strike without it.
	if (!IsValid(AbilityTargetActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	// The owning side moves Zed: the teleport is predicted and reaches the server with the movement.
	if (IsLocallyControlled())
	{
		RemoveTagFromAvatar(ZedUltimate::TAG_STATUS_COMBO_ABILITY4);
		TeleportToTarget();
	}

	UAbilityTask_PlayMontageAndWait* StrikeTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, StrikeMontage, /*Rate*/ 1.0f, NAME_None, /*bStopWhenAbilityEnds*/ true);
	StrikeTask->OnCompleted.AddDynamic(this, &ThisClass::OnStrikeCompleted);
	StrikeTask->ReadyForActivation();

	StartMark();

	// Pressing the ability again goes back to the shadow. Both sides wait for the input: the client's press reaches
	// the server with its prediction key, so both run the recast in the same prediction window.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/ true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Zed_Ability4::OnMontageCanceled()
{
	// A cast cut short ends the ultimate: Zed must not stay out of the world, stopped, until the shadow goes away.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
}

void UGA_Zed_Ability4::StartMark()
{
	// From now on the damage Zed deals to the target is stored.
	if (UAssassinsHealthComponent* TargetHealth = UAssassinsHealthComponent::FindHealthComponent(AbilityTargetActor))
	{
		TargetHealth->OnHealthChanged.AddDynamic(this, &ThisClass::StoreDamage);
		MarkedHealthComponent = TargetHealth;
	}

	bDetonationPending = true;

	UWorld* World = GetWorld();
	if ((World != nullptr) && (DamageEffectDelay > 0.0))
	{
		World->GetTimerManager().SetTimer(DetonationTimerHandle, this, &ThisClass::DeathMarkDetonation, static_cast<float>(DamageEffectDelay), /*bLoop*/ false);
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
	// The mark stops storing as it bursts.
	UnbindMarkedHealth();

	// The server waits for the client to reach the burst as well.
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
	// The one who clicked goes on attacking the target.
	if (IsLocallyControlled() && IsValid(AbilityTargetActor))
	{
		FGameplayEventData Payload;
		Payload.Target = AbilityTargetActor;
		SendGameplayEvent(ZedUltimate::TAG_EVENT_ATTACK, Payload);
	}
}

void UGA_Zed_Ability4::OnRecastReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, /*bTestAlreadyPressed*/ true);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastPressed);
	PressTask->ReadyForActivation();
}

void UGA_Zed_Ability4::OnRecastPressed(float TimeWaited)
{
	if (!IsValid(SpawnedShadow))
	{
		// Nothing to go back to. The mark still bursts if it has not yet.
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
	// The owning client bursts on its own timer without waiting for the server(OnlyServerWait). Ended there, once Zed went
	// back, its end would reach the server before the server's burst and cut it off.
	if (!K2_HasAuthority())
	{
		return;
	}

	if (HasMovedToShadow && !bDetonationPending)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
	}
}

void UGA_Zed_Ability4::OnShadowEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	// Only this activation's shadow counts.
	if (Actor != SpawnedShadow)
	{
		return;
	}

	UnbindShadow();
	SpawnedShadow = nullptr;

	// There is nothing to go back to anymore. The mark still bursts if it has not yet.
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

	// Whatever Zed was attacking stays behind.
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

	// Behind the target, on the line from Zed through it. Only the XY plane counts.
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = AbilityTargetActor->GetActorLocation();
	const FRotator ToTarget = UKismetMathLibrary::FindLookAtRotation(FVector(AvatarLocation.X, AvatarLocation.Y, 0.0), FVector(TargetLocation.X, TargetLocation.Y, 0.0));
	const double TeleportDistance = FVector::Dist2D(AvatarLocation, TargetLocation) + ZedUltimate::BehindTargetDistance;
	const FVector TeleportLocation = AvatarLocation + (ToTarget.Vector() * TeleportDistance);

	// Facing the target from there.
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
	// Only the channel of this cast.
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
	// What the ability bound on other objects would otherwise reach its next activation.
	UnbindChannelingEnded();
	UnbindMarkedHealth();
	UnbindShadow();
	SpawnedShadow = nullptr;

	// The ability object is kept for the next activation: a timer left running would go off in it.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EffectEventTimerHandle);
		World->GetTimerManager().ClearTimer(DetonationTimerHandle);
	}

	// However the ultimate ends, Zed is back in the world.
	SetCapsuleCollisionEnabled(true);

	StoredDamage = 0.0;
	HasMovedToShadow = false;
	IsDamageDone = false;
	bDetonationPending = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
