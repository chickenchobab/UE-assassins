#include "AbilitySystem/Abilities/Attack/Ability_Attack.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/AssassinsTargetChasingComponent.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "AbilitySystem/Tasks/AbilityTask_CustomizeTickTask.h"
#include "Animation/AnimMontage.h"
#include "Character/AssassinsCharacter.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "Teams/AssassinsTeamBaseActor.h"
#include "TimerManager.h"

namespace AttackTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ACTIVATE_ATTACK, "Event.ActivateAttack");

	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ACTIVATE_ATTACK, "Ability.ActivateAttack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ACTIVATING_ATTACK, "Status.Ability.ActivatingAttack");

	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ATTACK, "Status.Combo.Attack");
};

namespace
{
	// UAbilitySystemBlueprintLibrary::IsActiveGameplayEffectHandleActive는 이 모듈로 export되지 않아 같은 일을 직접 한다.
	bool IsGameplayEffectHandleActive(const FActiveGameplayEffectHandle& Handle)
	{
		const UAbilitySystemComponent* OwningASC = Handle.GetOwningAbilitySystemComponent();
		return Handle.IsValid() && OwningASC && (OwningASC->GetActiveGameplayEffect(Handle) != nullptr);
	}

	// 몽타주는 끝나기 이만큼 전에 블렌드아웃을 시작한다. 재생 속도와 상관없는 실제 시간이다(FAnimMontageInstance::Advance).
	float GetBlendOutTriggerTime(const UAnimMontage* Montage)
	{
		if (!Montage->bEnableAutoBlendOut)
		{
			return 0.0f;
		}
		return (Montage->BlendOutTriggerTime >= 0.0f) ? Montage->BlendOutTriggerTime : Montage->BlendOut.GetBlendTime();
	}
}

UAbility_Attack::UAbility_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanTargetStructure = true;
}

void UAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UpdateTargetChasingCondition();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive() && IsLocallyControlled())
	{
		GetReadyToTargetChange();
	}
}

void UAbility_Attack::UpdateTargetChasingCondition()
{
	if (!K2_HasAuthority())
	{
		return;
	}

	UAbilityTask_CustomizeTickTask* TickTask = UAbilityTask_CustomizeTickTask::CustomizeTickTask(this);
	TickTask->OnTickTask.AddDynamic(this, &ThisClass::OnChasingTick);
	TickTask->ReadyForActivation();
}

void UAbility_Attack::OnChasingTick(float DeltaTime)
{
	if (UAssassinsTargetChasingComponent* ChasingComponent = GetTargetChasingComponentFromController())
	{
		ChasingComponent->SetKeepChase(ShouldKeepFollowingTarget());
	}
}

void UAbility_Attack::GetReadyToTargetChange()
{
	UAbilityTask_WaitInputRelease* WaitReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	WaitReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
	WaitReleaseTask->ReadyForActivation();
}

void UAbility_Attack::OnInputReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* WaitPressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, true);
	WaitPressTask->OnPress.AddDynamic(this, &ThisClass::OnInputPressed);
	WaitPressTask->ReadyForActivation();
}

void UAbility_Attack::OnInputPressed(float TimeWaited)
{
	AActor* CursorTarget = GetCurrentCursorTarget();

	if ((CursorTarget != AbilityTargetActor) && (CursorTarget != nullptr) &&
		(CursorTarget->IsA<AAssassinsCharacter>() || CursorTarget->IsA<AAssassinsTeamBaseActor>()))
	{
		HandleAttackTargetChanged(CursorTarget);
		return;
	}

	GetReadyToTargetChange();
}

void UAbility_Attack::HandleAttackTargetChanged(AActor* NewTarget)
{
	ResetTargetState();
	StopControllerMove();
	CancelActivateAttack();

	// 공격 유지 어빌리티는 바로 위에서 취소되어, 이벤트는 다음 틱에야 다시 시작한다.
	PendingAttackTarget = NewTarget;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::SendPendingAttackEvent);
	}
}

void UAbility_Attack::SendPendingAttackEvent()
{
	SendActivateAttackEvent(PendingAttackTarget);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAbility_Attack::SendActivateAttackEvent(AActor* TargetActor)
{
	// 공격 유지 어빌리티는 로컬 전용이라, 엔진은 아바타를 조종하는 쪽에서만 이벤트로 시작한다
	// (UAbilitySystemComponent::HasNetworkAuthorityToActivateTriggeredAbility). 다른 곳에서 보낸 이벤트는 아무것도 시작하지 않는다.
	if (!IsLocallyControlled())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.Target = TargetActor;

	SendGameplayEvent(AttackTags::TAG_EVENT_ACTIVATE_ATTACK, Payload);
}

void UAbility_Attack::CancelActivateAttack()
{
	if (UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponentFromActorInfo())
	{
		AssassinsASC->K2_CancelAbilities(AttackTags::TAG_ABILITY_ACTIVATE_ATTACK, FGameplayTag());
	}
}

void UAbility_Attack::OnServerReachedToTarget()
{
	// Super를 부르지 않는다. 공격은 대상에게 다가가는 데도 쓰여 커밋하지 않는다.
	StartWhenAttackReady();
}

void UAbility_Attack::OnNetSync()
{
	PlayMontage();
}

void UAbility_Attack::OnMoveComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (!IsActive())
	{
		return;
	}

	if (Result != EPathFollowingResult::Success)
	{
		ResetTargetState();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAssassinsTargetChasingComponent* ChasingComponent = GetTargetChasingComponentFromController();
	if (!IsValid(ChasingComponent))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (ChasingComponent->ShouldKeepChase())
	{
		return;
	}

	ResetTargetState();

	if (IsLocallyControlled())
	{
		StartWhenAttackReady();
		return;
	}

	ClientSetReplicatedEvent(EAbilityCustomReplicatedEvent::GameCustom1);
	WaitForNetSync();
}

void UAbility_Attack::StartWhenAttackReady()
{
	// 후딜을 끊고 다시 눌러도 앞 공격의 주기가 끝나기 전에는 선딜을 시작하지 않는다. 주기는 조종하는 쪽이 자기 시각으로 지킨다.
	// 원격 클라는 동기화 전에 기다려서, 서버가 동기화에서 클라를 기다리고 양쪽이 함께 선딜을 시작한다. 연속 공격은 주기가 끝날 때
	// 이어져 기다리지 않는다(AdjustMontageRateAndEventTime).
	const UWorld* World = GetWorld();
	const double Remaining = World ? (NextAttackTime - World->GetTimeSeconds()) : 0.0;
	if (Remaining > UE_KINDA_SMALL_NUMBER)
	{
		UAbilityTask_WaitDelay* ReadyTask = UAbilityTask_WaitDelay::WaitDelay(this, static_cast<float>(Remaining));
		ReadyTask->OnFinish.AddDynamic(this, &ThisClass::OnAttackReady);
		ReadyTask->ReadyForActivation();
		return;
	}

	if (K2_HasAuthority())
	{
		PlayMontage();
		return;
	}

	WaitForNetSync();
}

void UAbility_Attack::OnAttackReady()
{
	StartWhenAttackReady();
}

void UAbility_Attack::PlayMontage()
{
	PrePlayMontage();

	if (!IsActive())
	{
		return;
	}

	// PrePlayMontage가 적용한 쿨다운에 맞추려고 몽타주를 일부러 다시 고른다.
	SetMontageToPlay();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, static_cast<float>(MontageRate), NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCanceled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCanceled);
	MontageTask->ReadyForActivation();

	// 재생에 실패한 몽타주는 ReadyForActivation 안에서 바로 취소되어 어빌리티가 이미 끝났을 수 있다. 끝난 뒤에는 명중 타이머가 남으면 안 된다.
	if (IsActive())
	{
		PostPlayMontage();
	}
}

void UAbility_Attack::PrePlayMontage()
{
	AttackCooldownActiveHandle = BP_ApplyGameplayEffectToOwner(AttackCooldownEffectClass, 1, 1);
	AddTagToAvatar(AttackTags::TAG_STATUS_COMBO_ATTACK);

	Super::PrePlayMontage();
}

void UAbility_Attack::PostPlayMontage()
{
	if (UWorld* World = GetWorld())
	{
		NextAttackTime = World->GetTimeSeconds() + AttackPeriod;
		World->GetTimerManager().SetTimer(AttackHitTimerHandle, this, &ThisClass::OnHitTimerElapsed, static_cast<float>(HitEventTime), false);
	}
}

void UAbility_Attack::OnHitTimerElapsed()
{
	// 끝난 어빌리티가 명중하면 클라이언트가 혼자 싱크 포인트를 지나 데미지를 준다.
	if (!IsActive())
	{
		return;
	}

	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnHitNetSync);
	SyncTask->ReadyForActivation();
}

void UAbility_Attack::OnHitNetSync()
{
	OnAttackHit();
}

void UAbility_Attack::OnAttackHit()
{
	RemoveTagFromAvatar(AttackTags::TAG_STATUS_COMBO_ATTACK);
}

void UAbility_Attack::OnMontageComplete()
{
	HandleMontageEnded();
}

void UAbility_Attack::OnMontageCanceled()
{
	HandleMontageEnded();
}

void UAbility_Attack::HandleMontageEnded()
{
	IsMontageEnded = true;

	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter && AvatarCharacter->HasGameplayTag(AttackTags::TAG_STATUS_ABILITY_ACTIVATING_ATTACK))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UAbility_Attack::SetMontageToPlay()
{
	Super::SetMontageToPlay();

	AdjustMontageRateAndEventTime();
}

void UAbility_Attack::AdjustMontageRateAndEventTime()
{
	if (!IsValid(MontageToPlay))
	{
		return;
	}

	AttackPeriod = 0.0;
	if (IsGameplayEffectHandleActive(AttackCooldownActiveHandle))
	{
		AttackPeriod = UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectRemainingDuration(this, AttackCooldownActiveHandle);
	}
	else
	{
		bool bFoundAttackSpeed = false;
		const float AttackSpeed = UAbilitySystemBlueprintLibrary::GetFloatAttribute(GetAvatarActorFromActorInfo(), UAssassinsCombatSet::GetAttackSpeedAttribute(), bFoundAttackSpeed);
		if (bFoundAttackSpeed && (AttackSpeed > 0.0f))
		{
			AttackPeriod = 1.0 / AttackSpeed;
		}
	}

	// 다음 공격은 이 몽타주가 블렌드아웃을 시작할 때 이어진다(HandleMontageEnded, GA_ActivateAttack). 블렌드아웃 시작이 공격 주기의
	// 끝에 오도록 블렌드아웃만큼 길게 재생해, 다음 동작이 섞여 들어오면서도 주기는 공격 속도를 따른다.
	if (AttackPeriod > 0.0)
	{
		MontageRate = MontageToPlay->GetPlayLength() / (AttackPeriod + GetBlendOutTriggerTime(MontageToPlay));
	}

	if (MontageRate > 0.0)
	{
		HitEventTime = HitEventTime / MontageRate;
	}
}

bool UAbility_Attack::ShouldKeepFollowingTarget() const
{
	const UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponentFromActorInfo();
	if (AssassinsASC == nullptr)
	{
		return false;
	}

	return AssassinsASC->HasAnyMatchingGameplayTags(FGameplayTagContainer::CreateFromArray(ActionBlockingTags));
}

TSubclassOf<UGameplayEffect> UAbility_Attack::GetAttackEffectClass() const
{
	return AttackEffectClass;
}

void UAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		CancelActivateAttack();
	}

	// 명중 전에 끝나는 공격은 주기를 쓰지 않는다. Status.Combo.Attack은 명중(OnAttackHit)에서 떨어져서, 남아 있으면 이쪽에서는
	// 명중하지 않은 것이다. 각자 자기 명중으로 판단한다: 조종하는 쪽은 다음 공격을 바로 허용하고, 쿨다운 효과는 피해를 주는 서버가
	// 뗀다(클라는 동기화 뒤 예측 창 밖이라 쿨다운을 걸지 못하고 서버의 것을 복제받는다). 태그는 Super에서 떨어지므로 그 전에 본다.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(AttackTags::TAG_STATUS_COMBO_ATTACK))
	{
		NextAttackTime = 0.0;
		if (IsGameplayEffectHandleActive(AttackCooldownActiveHandle))
		{
			ASC->RemoveActiveGameplayEffect(AttackCooldownActiveHandle);
		}
	}

	IsMontageEnded = false;
	ResetTargetState();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
