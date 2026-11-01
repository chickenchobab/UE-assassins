#include "AbilitySystem/Abilities/Foundation/Ability_LocationTargeted.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Kismet/KismetMathLibrary.h"

UAbility_LocationTargeted::UAbility_LocationTargeted(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UAbility_LocationTargeted::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (TriggerEventData == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	EvaluateCursorTransform(*TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayMontage();
}

void UAbility_LocationTargeted::EvaluateCursorTransform(const FGameplayEventData& EventData)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return;
	}

	const FVector AvatarLocation = AvatarActor->GetActorLocation();

	const FHitResult* CursorHitResult = EventData.ContextHandle.GetHitResult();
	CursorLocation = CursorHitResult ? CursorHitResult->Location : FVector::ZeroVector;

	// 커서를 보는 건 XY 평면 기준이다. 커서가 아바타 바로 위면(돌진해 올라선 대상) 볼 방향이 없어, X축으로 돌지 않고 지금 방향을 유지한다.
	const double DistanceToCursor = FVector::Dist2D(AvatarLocation, CursorLocation);
	CursorLookAtRotation = (DistanceToCursor >= 1.0)
		? UKismetMathLibrary::FindLookAtRotation(FVector(AvatarLocation.X, AvatarLocation.Y, 0.0), FVector(CursorLocation.X, CursorLocation.Y, 0.0))
		: FRotator(0.0, AvatarActor->GetActorRotation().Yaw, 0.0);

	CursorLocationClamped = AvatarLocation + CursorLookAtRotation.Vector() * FMath::Clamp(DistanceToCursor, 0.0, AbilityRange);
}

void UAbility_LocationTargeted::PlayMontage()
{
	PrePlayMontage();

	if (!IsActive())
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, 1.0f, NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// 재생에 실패한 몽타주는 ReadyForActivation 안에서 바로 취소되고 어빌리티가 끝난다.
	if (IsActive())
	{
		PostPlayMontage();
	}
}

void UAbility_LocationTargeted::PrePlayMontage()
{
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		AvatarActor->SetActorRotation(CursorLookAtRotation, ETeleportType::TeleportPhysics);
	}
}

void UAbility_LocationTargeted::HandleMontageCompleted()
{
	OnMontageComplete();
}

void UAbility_LocationTargeted::HandleMontageCancelled()
{
	OnMontageCancelled();
}

void UAbility_LocationTargeted::OnMontageComplete()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAbility_LocationTargeted::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
