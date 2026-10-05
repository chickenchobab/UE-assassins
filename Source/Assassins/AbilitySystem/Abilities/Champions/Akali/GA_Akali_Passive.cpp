// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Passive.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/Tasks/AbilityTask_CustomizeTickTask.h"
#include "AbilitySystemComponent.h"
#include "Character/AssassinsChampion.h"
#include "Character/AssassinsCharacter.h"
#include "Engine/CurveTable.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"

namespace AkaliPassive
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_SPAWN_PASSIVE_RING, "Event.Champion.Akali.SpawnPassiveRing");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_PASSIVE_RING, "GameplayCue.Champion.Akali.PassiveRing");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYEFFECT_GAIN_SPEED, "GameplayEffect.Champion.Akali.GainSpeed");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_CAN_GAIN_SPEED, "Status.Champion.Akali.CanGainSpeed");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_PASSIVE, "Status.Combo.Passive");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY1, "Status.Combo.Ability1");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY2, "Status.Combo.Ability2");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY3, "Status.Combo.Ability3");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY4, "Status.Combo.Ability4");

	// The ring is centered between the champion hit and Akali, this far from the champion.
	static constexpr double RingOffsetTowardAvatar = 80.0;

	// The enemy detection sweeps up from Akali's location by this much.
	static constexpr double EnemyDetectionHeight = 100.0;

	// Directions are taken on the ground, like Vector_Normal2D with its tolerance.
	static constexpr double DirectionTolerance = 1.e-4;
};

UGA_Akali_Passive::UGA_Akali_Passive(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RingDurationRow = TEXT("Passive.RingDuration");
}

void UGA_Akali_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (K2_HasAuthority())
	{
		// Sent by the server only, from the execution of Akali's ability damage.
		UAbilityTask_WaitGameplayEvent* RingEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, AkaliPassive::TAG_EVENT_SPAWN_PASSIVE_RING, /*OptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ false, /*OnlyMatchExact*/ true);
		RingEventTask->EventReceived.AddDynamic(this, &ThisClass::OnSpawnRingEvent);
		RingEventTask->ReadyForActivation();

		UAbilityTask_CustomizeTickTask* SpeedTask = UAbilityTask_CustomizeTickTask::CustomizeTickTask(this);
		SpeedTask->OnTickTask.AddDynamic(this, &ThisClass::OnSpeedTick);
		SpeedTask->ReadyForActivation();
	}
	else if (IsLocallyControlled())
	{
		// The owning client does not take the montages the server replicates: it plays the passive's own as the
		// replicated status comes on.
		UAbilityTask_WaitGameplayTagAdded* PassiveStatusTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
			this, AkaliPassive::TAG_STATUS_COMBO_PASSIVE, /*InOptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ false);
		PassiveStatusTask->Added.AddDynamic(this, &ThisClass::OnPassiveStatusAdded);
		PassiveStatusTask->ReadyForActivation();
	}
}

void UGA_Akali_Passive::OnSpawnRingEvent(FGameplayEventData Payload)
{
	// The champion hit. Without it the current ring, if any, stays as it is.
	const AActor* TargetActor = Payload.Target.Get();
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(TargetActor) || (AvatarActor == nullptr))
	{
		return;
	}

	ClearRingTasks();

	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FVector ToAvatar = (AvatarActor->GetActorLocation() - TargetLocation).GetSafeNormal2D(AkaliPassive::DirectionTolerance);
	RingLocation = TargetLocation + (ToAvatar * AkaliPassive::RingOffsetTowardAvatar);

	SpawnRing();
}

void UGA_Akali_Passive::SpawnRing()
{
	// The cue carries the location to every client.
	K2_RemoveGameplayCue(AkaliPassive::TAG_GAMEPLAYCUE_PASSIVE_RING);

	FGameplayCueParameters CueParameters;
	CueParameters.Location = RingLocation;
	K2_AddGameplayCueWithParams(AkaliPassive::TAG_GAMEPLAYCUE_PASSIVE_RING, CueParameters, /*bRemoveOnAbilityEnd*/ true);

	BP_ApplyGameplayEffectToOwner(CanGainSpeedEffectClass, /*GameplayEffectLevel*/ 1, /*Stacks*/ 1);

	RingDistanceCheckingTask = UAbilityTask_CustomizeTickTask::CustomizeTickTask(this);
	RingDistanceCheckingTask->OnTickTask.AddDynamic(this, &ThisClass::OnRingTick);
	RingDistanceCheckingTask->ReadyForActivation();

	const float RingDuration = EvaluateCurveTableRowByAbilityLevel(AbilityMagnitudeTable, RingDurationRow, TEXT("Akali passive ring duration"));
	RingDurationTask = UAbilityTask_WaitDelay::WaitDelay(this, RingDuration);
	RingDurationTask->OnFinish.AddDynamic(this, &ThisClass::OnRingDurationFinished);
	RingDurationTask->ReadyForActivation();
}

void UGA_Akali_Passive::ClearRingTasks()
{
	// Ending the tick task from its own tick is safe: the tasks component ticks a copy of its list.
	if (IsValid(RingDistanceCheckingTask))
	{
		RingDistanceCheckingTask->EndTask();
	}
	RingDistanceCheckingTask = nullptr;

	if (IsValid(RingDurationTask))
	{
		RingDurationTask->EndTask();
	}
	RingDurationTask = nullptr;
}

void UGA_Akali_Passive::OnRingTick(float DeltaTime)
{
	if (!IsAvatarOutOfRingRadius())
	{
		return;
	}

	// Akali left the ring: the passive comes on.
	ClearRingTasks();
	K2_RemoveGameplayCue(AkaliPassive::TAG_GAMEPLAYCUE_PASSIVE_RING);
	ActivatePassive();
}

void UGA_Akali_Passive::OnRingDurationFinished()
{
	// Akali stayed in the ring: it fades without the passive.
	ClearRingTasks();
	K2_RemoveGameplayCue(AkaliPassive::TAG_GAMEPLAYCUE_PASSIVE_RING);
}

void UGA_Akali_Passive::ActivatePassive()
{
	BP_ApplyGameplayEffectToOwner(PassiveStatusEffectClass, /*GameplayEffectLevel*/ 1, /*Stacks*/ 1);
	BP_ApplyGameplayEffectToOwner(CanGainSpeedEffectClass, /*GameplayEffectLevel*/ 1, /*Stacks*/ 1);

	// Replicated to everyone but the owning client, which plays its own(OnPassiveStatusAdded).
	PlayPassiveMontage();
}

void UGA_Akali_Passive::OnPassiveStatusAdded()
{
	PlayPassiveMontage();
}

void UGA_Akali_Passive::PlayPassiveMontage()
{
	if ((PassiveMontage == nullptr) || !CanPlayPassiveActivatingMontage())
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, PassiveMontage, /*Rate*/ 1.0f, NAME_None, /*bStopWhenAbilityEnds*/ true);
	MontageTask->ReadyForActivation();
}

void UGA_Akali_Passive::OnSpeedTick(float DeltaTime)
{
	UpdateSpeed();
}

void UGA_Akali_Passive::UpdateSpeed()
{
	bool bShouldGainSpeed = false;

	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter && AvatarCharacter->HasGameplayTag(AkaliPassive::TAG_STATUS_CAN_GAIN_SPEED))
	{
		bShouldGainSpeed = AvatarCharacter->HasGameplayTag(AkaliPassive::TAG_STATUS_COMBO_PASSIVE) ? IsAvatarMovingTowardEnemy() : IsAvatarGettingOutOfRing();
	}

	// The speed only changes when it has to, not every frame.
	const bool bGainingSpeed = IsSpeedEffectActive();
	if (bShouldGainSpeed && !bGainingSpeed)
	{
		SpeedEffectHandle = BP_ApplyGameplayEffectToOwner(GainSpeedEffectClass, /*GameplayEffectLevel*/ 1, /*Stacks*/ 1);
	}
	else if (!bShouldGainSpeed && bGainingSpeed)
	{
		RemoveSpeed();
	}
}

void UGA_Akali_Passive::RemoveSpeed()
{
	// By the asset tag, as the blueprint did: whatever speed of the passive is on goes.
	BP_RemoveGameplayEffectFromOwnerWithAssetTags(FGameplayTagContainer(AkaliPassive::TAG_GAMEPLAYEFFECT_GAIN_SPEED), /*StacksToRemove*/ -1);
	SpeedEffectHandle.Invalidate();
}

bool UGA_Akali_Passive::IsSpeedEffectActive() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return SpeedEffectHandle.IsValid() && ASC && (ASC->GetActiveGameplayEffect(SpeedEffectHandle) != nullptr);
}

bool UGA_Akali_Passive::IsAvatarOutOfRingRadius() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor && (FVector::Dist2D(RingLocation, AvatarActor->GetActorLocation()) >= RingRadius);
}

bool UGA_Akali_Passive::CanPlayPassiveActivatingMontage() const
{
	// Not over the montage of an ability Akali is in the middle of.
	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter == nullptr)
	{
		return false;
	}

	return !(AvatarCharacter->HasGameplayTag(AkaliPassive::TAG_STATUS_COMBO_ABILITY1)
		|| AvatarCharacter->HasGameplayTag(AkaliPassive::TAG_STATUS_COMBO_ABILITY2)
		|| AvatarCharacter->HasGameplayTag(AkaliPassive::TAG_STATUS_COMBO_ABILITY3)
		|| AvatarCharacter->HasGameplayTag(AkaliPassive::TAG_STATUS_COMBO_ABILITY4));
}

bool UGA_Akali_Passive::IsAvatarGettingOutOfRing() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return false;
	}

	const FVector Velocity = AvatarActor->GetVelocity();
	if (Velocity.IsNearlyZero(AkaliPassive::DirectionTolerance))
	{
		return false;
	}

	const FVector ToRing = (RingLocation - AvatarActor->GetActorLocation()).GetSafeNormal2D(AkaliPassive::DirectionTolerance);
	return FVector::DotProduct(Velocity.GetSafeNormal2D(AkaliPassive::DirectionTolerance), ToRing) < 0.0;
}

bool UGA_Akali_Passive::IsAvatarMovingTowardTarget(const AActor* TargetActor) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if ((AvatarActor == nullptr) || (TargetActor == nullptr))
	{
		return false;
	}

	const FVector Velocity = AvatarActor->GetVelocity();
	if (Velocity.IsNearlyZero(AkaliPassive::DirectionTolerance))
	{
		return false;
	}

	const FVector MoveDirection = Velocity.GetSafeNormal2D(AkaliPassive::DirectionTolerance);
	const FVector ToTarget = (TargetActor->GetActorLocation() - AvatarActor->GetActorLocation()).GetSafeNormal2D(AkaliPassive::DirectionTolerance);

	// Clamped: rounding may put the cosine of two unit vectors just past 1, where Acos has no value.
	const double CosineAngle = FMath::Clamp(FVector::DotProduct(MoveDirection, ToTarget), -1.0, 1.0);
	return FMath::RadiansToDegrees(FMath::Acos(CosineAngle)) <= MoveTowardDegree;
}

bool UGA_Akali_Passive::IsAvatarMovingTowardEnemy() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const UWorld* World = GetWorld();
	if ((AvatarActor == nullptr) || (World == nullptr))
	{
		return false;
	}

	const FVector Start = AvatarActor->GetActorLocation();
	const FVector End = Start + FVector(0.0, 0.0, AkaliPassive::EnemyDetectionHeight);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AkaliPassiveEnemyDetection), /*bTraceComplex*/ false, AvatarActor);

	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(EnemyDetectionRadius), QueryParams);

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (IsValidEnemy(HitActor) && HitActor->IsA<AAssassinsChampion>() && IsAvatarMovingTowardTarget(HitActor))
		{
			return true;
		}
	}

	return false;
}

void UGA_Akali_Passive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// The speed is infinite and the effects outlive the avatar: it must not stay after the passive, e.g. over a death.
	// Removing needs authority, so this does nothing on the client.
	if (IsSpeedEffectActive())
	{
		RemoveSpeed();
	}
	SpeedEffectHandle.Invalidate();

	// The tasks end with the ability, and the ring cue is removed with it.
	RingDistanceCheckingTask = nullptr;
	RingDurationTask = nullptr;
	RingLocation = FVector::ZeroVector;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
