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
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

bool UGA_UnitTargeted::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	// Super를 부르지 않는다. Super는 이 클래스가 대신하는 블루프린트 구현만 부른다.
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
	if (TriggerEventData == nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	CachedTargetChasingComponent->StopChase();

	// 사거리 안의 대상은 바로 시전한다. 시전은 다른 쪽을 기다린 뒤가 아니라 지금부터 붙든다. 사거리 밖이면 도착부터 붙든다.
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
		FAbilityReplicatedDelegate ReachedTargetDelegate;
		ReachedTargetDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UGA_UnitTargeted, OnServerReachedToTarget));
		CallOrAddReplicatedDelegate(EAbilityCustomReplicatedEvent::GameCustom1, ReachedTargetDelegate, true);
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
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
	SetCanBeCanceled(false);

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	PlayMontage();
}

void UGA_UnitTargeted::PlayMontage()
{
	PrePlayMontage();

	// 여기서부터 시전이 붙든다. 양쪽이 붙인 태그가 그사이 빠지지 않도록 나중에 뗀다.
	RemovePendingCastTags();

	if (!IsActive())
	{
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, static_cast<float>(MontageRate), NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCanceled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCanceled);
	MontageTask->ReadyForActivation();

	// 재생에 실패한 몽타주는 ReadyForActivation 안에서 바로 취소되어 어빌리티가 이미 끝났을 수 있다.
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
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_UnitTargeted::OnMontageCanceled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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

	// 추적이 재는 방식과 같다(UAssassinsTargetChasingComponent::ChaseTarget). 도착은 대상 반지름도 세서, 여기서 사거리 안이면 바로 끝난다.
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

	// 어빌리티가 붙이는 태그(AddTagToAvatar)와 별개인 루즈 태그라 각각 한 번씩 떨어진다.
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

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	return UKismetMathLibrary::FindLookAtRotation(FVector(AvatarLocation.X, AvatarLocation.Y, 0.0), FVector(TargetLocation.X, TargetLocation.Y, 0.0));
}

void UGA_UnitTargeted::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemovePendingCastTags();

	// 추적 컴포넌트가 어빌리티보다 오래 살아서, 다른 어빌리티가 시작한 추적이 여기로 돌아오면 안 된다.
	if (IsValid(CachedTargetChasingComponent))
	{
		CachedTargetChasingComponent->HandleChaseCompleted.RemoveDynamic(this, &ThisClass::OnMoveComplete);
	}
	CachedTargetChasingComponent = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
