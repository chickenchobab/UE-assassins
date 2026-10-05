// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Foundation/GA_UnitTargeted.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/AssassinsTargetChasingComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/AssassinsChampion.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/AssassinsPlayerController.h"

UGA_UnitTargeted::UGA_UnitTargeted(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The ability talks to the other side through replicated events.
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

bool UGA_UnitTargeted::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	// Not calling Super: it only answers the blueprint implementation, which this class replaces.
	if (Payload == nullptr)
	{
		return false;
	}

	AActor* EventTargetActor = const_cast<AActor*>(ToRawPtr(Payload->Target));
	if (!IsValidEnemy(EventTargetActor))
	{
		return false;
	}

	return !ShouldTargetChampion || (EventTargetActor && EventTargetActor->IsA<AAssassinsChampion>());
}

void UGA_UnitTargeted::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// The target comes with the event, there is nothing to aim at without it. Not a cancellation: nothing was going on.
	if (TriggerEventData == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	AbilityTargetActor = const_cast<AActor*>(ToRawPtr(TriggerEventData->Target));

	MoveToTarget();
}

void UGA_UnitTargeted::MoveToTarget()
{
	CachedTargetChasingComponent = GetTargetChasingComponentFromController();
	if (!IsValid(CachedTargetChasingComponent))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	// Abort what the previous ability was chasing.
	CachedTargetChasingComponent->StopChase();

	// A target in range is cast at once for the player: the cast holds from now, not from after the wait for the other
	// side. Out of range, from the arrival.
	if (IsTargetInRange())
	{
		AddPendingCastTags();
	}

	if (K2_HasAuthority())
	{
		CachedTargetChasingComponent->HandleChaseCompleted.AddDynamic(this, &ThisClass::OnMoveComplete);
	}
	else
	{
		// The client goes on when the server says the avatar arrived.
		FAbilityReplicatedDelegate ReachedTargetDelegate;
		ReachedTargetDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UGA_UnitTargeted, OnServerReachedToTarget));
		CallOrAddReplicatedDelegate(EAbilityCustomReplicatedEvent::GameCustom1, ReachedTargetDelegate, /*bUnbindCalledDelegate*/ true);
	}

	CachedTargetChasingComponent->ChaseTarget(AbilityTargetActor, static_cast<float>(AbilityRange));
}

void UGA_UnitTargeted::OnMoveComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (!IsActive())
	{
		return;
	}

	ResetTargetState();

	if ((Result != EPathFollowingResult::Success) || !IsValid(AbilityTargetActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	AddPendingCastTags();
	ClientSetReplicatedEvent(EAbilityCustomReplicatedEvent::GameCustom1);
	WaitForNetSync();
}

void UGA_UnitTargeted::OnServerReachedToTarget()
{
	AddPendingCastTags();
	WaitForNetSync();
}

void UGA_UnitTargeted::WaitForNetSync()
{
	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::BothWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnNetSync);
	SyncTask->ReadyForActivation();
}

void UGA_UnitTargeted::OnNetSync()
{
	// The ability logic starts here, a click can't cancel it anymore.
	SetCanBeCanceled(false);

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ true);
		return;
	}

	PlayMontage();
}

void UGA_UnitTargeted::PlayMontage()
{
	PrePlayMontage();

	// The cast holds what it holds from here. Taken off after, so that a tag both put on stays on in between.
	RemovePendingCastTags();

	// PrePlayMontage ends the ability when the target is gone.
	if (!IsActive())
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, static_cast<float>(MontageRate), NAME_None, /*bStopWhenAbilityEnds*/ true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCanceled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCanceled);
	MontageTask->ReadyForActivation();

	// A montage that fails to play is cancelled right inside ReadyForActivation, which may already have ended the ability.
	if (IsActive())
	{
		PostPlayMontage();
	}
}

void UGA_UnitTargeted::PrePlayMontage()
{
	SetMontageToPlay();

	if (!IsValid(AbilityTargetActor))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		AvatarActor->SetActorRotation(GetTargetLookAtRotation(AbilityTargetActor), ETeleportType::TeleportPhysics);
	}
}

void UGA_UnitTargeted::SetMontageToPlay()
{
	if (MontageDataArray.IsEmpty())
	{
		return;
	}

	const UAssassinsMontageWithTiming* SelectedMontage = MontageDataArray[FMath::RandRange(0, MontageDataArray.Num() - 1)];
	if (SelectedMontage == nullptr)
	{
		return;
	}

	MontageToPlay = SelectedMontage->Montage;
	HitEventTime = SelectedMontage->Timing;
}

void UGA_UnitTargeted::HandleMontageCompleted()
{
	OnMontageComplete();
}

void UGA_UnitTargeted::HandleMontageCanceled()
{
	OnMontageCanceled();
}

void UGA_UnitTargeted::OnMontageComplete()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_UnitTargeted::OnMontageCanceled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

bool UGA_UnitTargeted::ShouldKeepFollowingTarget() const
{
	return false;
}

FGameplayTagContainer UGA_UnitTargeted::GetPendingCastTags() const
{
	return FGameplayTagContainer();
}

bool UGA_UnitTargeted::IsTargetInRange() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if ((AvatarActor == nullptr) || !IsValid(AbilityTargetActor))
	{
		return false;
	}

	// As the chase measures it(UAssassinsTargetChasingComponent::ChaseTarget). Its arrival also counts the radius of the
	// target, so that in range here it is over at once.
	double Range = AbilityRange;
	if (!AbilityTargetActor->IsA<AAssassinsCharacter>())
	{
		Range += AbilityTargetActor->GetSimpleCollisionRadius();
	}
	return FVector::Dist2D(AvatarActor->GetActorLocation(), AbilityTargetActor->GetActorLocation()) <= Range;
}

void UGA_UnitTargeted::AddPendingCastTags()
{
	const FGameplayTagContainer PendingCastTags = GetPendingCastTags();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (bPendingCastTagsAdded || PendingCastTags.IsEmpty() || (ASC == nullptr))
	{
		return;
	}

	// Loose tags of their own, apart from those the ability puts on(AddTagToAvatar): each comes off once.
	ASC->AddLooseGameplayTags(PendingCastTags);
	bPendingCastTagsAdded = true;
}

void UGA_UnitTargeted::RemovePendingCastTags()
{
	if (!bPendingCastTagsAdded)
	{
		return;
	}

	bPendingCastTagsAdded = false;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTags(GetPendingCastTags());
	}
}

void UGA_UnitTargeted::StopControllerMove()
{
	if (AAssassinsPlayerController* AssassinsPlayerController = GetAssassinsPlayerControllerFromActorInfo())
	{
		AssassinsPlayerController->StopMovement();
	}
}

void UGA_UnitTargeted::ResetTargetState()
{
	if (const AController* OwningController = GetControllerFromActorInfo())
	{
		if (UAssassinsTargetChasingComponent* ChasingComponent = OwningController->FindComponentByClass<UAssassinsTargetChasingComponent>())
		{
			ChasingComponent->ResetTargetState();
		}
	}
}

FRotator UGA_UnitTargeted::GetTargetLookAtRotation(const AActor* TargetActor) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if ((AvatarActor == nullptr) || (TargetActor == nullptr))
	{
		return FRotator::ZeroRotator;
	}

	// Only the XY plane matters: the Z of both locations is dropped.
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	return UKismetMathLibrary::FindLookAtRotation(FVector(AvatarLocation.X, AvatarLocation.Y, 0.0), FVector(TargetLocation.X, TargetLocation.Y, 0.0));
}

void UGA_UnitTargeted::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Ended before its cast: a cancel, or a target gone.
	RemovePendingCastTags();

	// The chasing component outlives the ability: a chase another ability starts must not come back here.
	if (IsValid(CachedTargetChasingComponent))
	{
		CachedTargetChasingComponent->HandleChaseCompleted.RemoveDynamic(this, &ThisClass::OnMoveComplete);
	}
	CachedTargetChasingComponent = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
