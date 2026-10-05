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

	static constexpr double RingOffsetTowardAvatar = 80.0;

	static constexpr double EnemyDetectionHeight = 100.0;

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
		UAbilityTask_WaitGameplayEvent* RingEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, AkaliPassive::TAG_EVENT_SPAWN_PASSIVE_RING, nullptr, false, true);
		RingEventTask->EventReceived.AddDynamic(this, &ThisClass::OnSpawnRingEvent);
		RingEventTask->ReadyForActivation();

		UAbilityTask_CustomizeTickTask* SpeedTask = UAbilityTask_CustomizeTickTask::CustomizeTickTask(this);
		SpeedTask->OnTickTask.AddDynamic(this, &ThisClass::OnSpeedTick);
		SpeedTask->ReadyForActivation();
	}
	else if (IsLocallyControlled())
	{
		// 소유 클라이언트는 서버가 복제한 몽타주를 받지 않으므로, 복제된 상태가 켜질 때 자기 몽타주를 재생한다.
		UAbilityTask_WaitGameplayTagAdded* PassiveStatusTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
			this, AkaliPassive::TAG_STATUS_COMBO_PASSIVE, nullptr, false);
		PassiveStatusTask->Added.AddDynamic(this, &ThisClass::OnPassiveStatusAdded);
		PassiveStatusTask->ReadyForActivation();
	}
}

void UGA_Akali_Passive::OnSpawnRingEvent(FGameplayEventData Payload)
{
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
	K2_RemoveGameplayCue(AkaliPassive::TAG_GAMEPLAYCUE_PASSIVE_RING);

	FGameplayCueParameters CueParameters;
	CueParameters.Location = RingLocation;
	K2_AddGameplayCueWithParams(AkaliPassive::TAG_GAMEPLAYCUE_PASSIVE_RING, CueParameters, true);

	BP_ApplyGameplayEffectToOwner(CanGainSpeedEffectClass, 1, 1);

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
	// 태스크 컴포넌트는 목록의 사본을 틱하므로 틱 태스크를 자기 틱에서 끝내도 안전하다.
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

	ClearRingTasks();
	K2_RemoveGameplayCue(AkaliPassive::TAG_GAMEPLAYCUE_PASSIVE_RING);
	ActivatePassive();
}

void UGA_Akali_Passive::OnRingDurationFinished()
{
	ClearRingTasks();
	K2_RemoveGameplayCue(AkaliPassive::TAG_GAMEPLAYCUE_PASSIVE_RING);
}

void UGA_Akali_Passive::ActivatePassive()
{
	BP_ApplyGameplayEffectToOwner(PassiveStatusEffectClass, 1, 1);
	BP_ApplyGameplayEffectToOwner(CanGainSpeedEffectClass, 1, 1);

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
		this, NAME_None, PassiveMontage, 1.0f, NAME_None, true);
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

	const bool bGainingSpeed = IsSpeedEffectActive();
	if (bShouldGainSpeed && !bGainingSpeed)
	{
		SpeedEffectHandle = BP_ApplyGameplayEffectToOwner(GainSpeedEffectClass, 1, 1);
	}
	else if (!bShouldGainSpeed && bGainingSpeed)
	{
		RemoveSpeed();
	}
}

void UGA_Akali_Passive::RemoveSpeed()
{
	BP_RemoveGameplayEffectFromOwnerWithAssetTags(FGameplayTagContainer(AkaliPassive::TAG_GAMEPLAYEFFECT_GAIN_SPEED), -1);
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

	// 반올림으로 단위 벡터의 내적이 1을 조금 넘으면 Acos가 값을 내지 못하므로 자른다.
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

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AkaliPassiveEnemyDetection), false, AvatarActor);

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
	// 속도 효과는 무한이고 효과는 아바타보다 오래 남아, 패시브 뒤(예: 죽음)에 남으면 안 된다. 제거는 권한이 필요해 클라이언트에서는 아무 일도 하지 않는다.
	if (IsSpeedEffectActive())
	{
		RemoveSpeed();
	}
	SpeedEffectHandle.Invalidate();

	RingDistanceCheckingTask = nullptr;
	RingDurationTask = nullptr;
	RingLocation = FVector::ZeroVector;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
