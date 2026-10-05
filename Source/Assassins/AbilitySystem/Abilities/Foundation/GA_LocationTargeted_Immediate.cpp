// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Kismet/KismetMathLibrary.h"

UGA_LocationTargeted_Immediate::UGA_LocationTargeted_Immediate(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UGA_LocationTargeted_Immediate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Where the ability is cast comes with the event. Not a cancellation: nothing was going on.
	if (TriggerEventData == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	EvaluateCursorTransform(*TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
		return;
	}

	PlayMontage();
}

void UGA_LocationTargeted_Immediate::EvaluateCursorTransform(const FGameplayEventData& EventData)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return;
	}

	const FVector AvatarLocation = AvatarActor->GetActorLocation();

	// The cursor trace comes in the effect context of the event.
	const FHitResult* CursorHitResult = EventData.ContextHandle.GetHitResult();
	CursorLocation = CursorHitResult ? CursorHitResult->Location : FVector::ZeroVector;

	// Facing the cursor is a matter of the XY plane only. A cursor right on the avatar(standing on the target it dashed
	// onto) gives no way to face: the avatar keeps the way it faces, rather than turning to the X axis.
	const double DistanceToCursor = FVector::Dist2D(AvatarLocation, CursorLocation);
	CursorLookAtRotation = (DistanceToCursor >= 1.0)
		? UKismetMathLibrary::FindLookAtRotation(FVector(AvatarLocation.X, AvatarLocation.Y, 0.0), FVector(CursorLocation.X, CursorLocation.Y, 0.0))
		: FRotator(0.0, AvatarActor->GetActorRotation().Yaw, 0.0);

	CursorLocationClamped = AvatarLocation + CursorLookAtRotation.Vector() * FMath::Clamp(DistanceToCursor, 0.0, AbilityRange);
}

void UGA_LocationTargeted_Immediate::PlayMontage()
{
	PrePlayMontage();

	if (!IsActive())
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, /*Rate*/ 1.0f, NAME_None, /*bStopWhenAbilityEnds*/ true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	MontageTask->ReadyForActivation();

	// A montage that fails to play is cancelled right inside ReadyForActivation, which ends the ability.
	if (IsActive())
	{
		PostPlayMontage();
	}
}

void UGA_LocationTargeted_Immediate::PrePlayMontage()
{
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		AvatarActor->SetActorRotation(CursorLookAtRotation, ETeleportType::TeleportPhysics);
	}
}

void UGA_LocationTargeted_Immediate::HandleMontageCompleted()
{
	OnMontageComplete();
}

void UGA_LocationTargeted_Immediate::HandleMontageCancelled()
{
	OnMontageCancelled();
}

void UGA_LocationTargeted_Immediate::OnMontageComplete()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_LocationTargeted_Immediate::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}
