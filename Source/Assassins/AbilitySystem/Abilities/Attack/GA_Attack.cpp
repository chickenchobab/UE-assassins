#include "AbilitySystem/Abilities/Attack/GA_Attack.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
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
}

UGA_Attack::UGA_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanTargetStructure = true;
}

void UGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UpdateTargetChasingCondition();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive() && IsLocallyControlled())
	{
		GetReadyToTargetChange();
	}
}

void UGA_Attack::UpdateTargetChasingCondition()
{
	if (!K2_HasAuthority())
	{
		return;
	}

	UAbilityTask_CustomizeTickTask* TickTask = UAbilityTask_CustomizeTickTask::CustomizeTickTask(this);
	TickTask->OnTickTask.AddDynamic(this, &ThisClass::OnChasingTick);
	TickTask->ReadyForActivation();
}

void UGA_Attack::OnChasingTick(float DeltaTime)
{
	if (UAssassinsTargetChasingComponent* ChasingComponent = GetTargetChasingComponentFromController())
	{
		ChasingComponent->SetKeepChase(ShouldKeepFollowingTarget());
	}
}

void UGA_Attack::GetReadyToTargetChange()
{
	UAbilityTask_WaitInputRelease* WaitReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	WaitReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
	WaitReleaseTask->ReadyForActivation();
}

void UGA_Attack::OnInputReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* WaitPressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, true);
	WaitPressTask->OnPress.AddDynamic(this, &ThisClass::OnInputPressed);
	WaitPressTask->ReadyForActivation();
}

void UGA_Attack::OnInputPressed(float TimeWaited)
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

void UGA_Attack::HandleAttackTargetChanged(AActor* NewTarget)
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

void UGA_Attack::SendPendingAttackEvent()
{
	SendActivateAttackEvent(PendingAttackTarget);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Attack::SendActivateAttackEvent(AActor* TargetActor)
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

void UGA_Attack::CancelActivateAttack()
{
	if (UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponentFromActorInfo())
	{
		AssassinsASC->K2_CancelAbilities(AttackTags::TAG_ABILITY_ACTIVATE_ATTACK, FGameplayTag());
	}
}

void UGA_Attack::OnServerReachedToTarget()
{
	// Super를 부르지 않는다. 공격은 대상에게 다가가는 데도 쓰여 커밋하지 않는다.
	WaitForNetSync();
}

void UGA_Attack::OnNetSync()
{
	PlayMontage();
}

void UGA_Attack::OnMoveComplete(FAIRequestID RequestID, EPathFollowingResult::Type Result)
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
		PlayMontage();
		return;
	}

	ClientSetReplicatedEvent(EAbilityCustomReplicatedEvent::GameCustom1);
	WaitForNetSync();
}

void UGA_Attack::PlayMontage()
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

void UGA_Attack::PrePlayMontage()
{
	AttackCooldownActiveHandle = BP_ApplyGameplayEffectToOwner(AttackCooldownEffectClass, 1, 1);
	AddTagToAvatar(AttackTags::TAG_STATUS_COMBO_ATTACK);

	Super::PrePlayMontage();
}

void UGA_Attack::PostPlayMontage()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AttackHitTimerHandle, this, &ThisClass::OnHitTimerElapsed, static_cast<float>(HitEventTime), false);
	}
}

void UGA_Attack::OnHitTimerElapsed()
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

void UGA_Attack::OnHitNetSync()
{
	OnAttackHit();
}

void UGA_Attack::OnAttackHit()
{
	RemoveTagFromAvatar(AttackTags::TAG_STATUS_COMBO_ATTACK);
}

void UGA_Attack::OnMontageComplete()
{
	HandleMontageEnded();
}

void UGA_Attack::OnMontageCanceled()
{
	HandleMontageEnded();
}

void UGA_Attack::HandleMontageEnded()
{
	IsMontageEnded = true;

	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter && AvatarCharacter->HasGameplayTag(AttackTags::TAG_STATUS_ABILITY_ACTIVATING_ATTACK))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Attack::SetMontageToPlay()
{
	Super::SetMontageToPlay();

	AdjustMontageRateAndEventTime();
}

void UGA_Attack::AdjustMontageRateAndEventTime()
{
	if (!IsValid(MontageToPlay))
	{
		return;
	}

	if (IsGameplayEffectHandleActive(AttackCooldownActiveHandle))
	{
		const float RemainingCooldown = UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectRemainingDuration(this, AttackCooldownActiveHandle);
		if (RemainingCooldown > 0.0f)
		{
			MontageRate = MontageToPlay->GetPlayLength() / RemainingCooldown;
		}
	}
	else
	{
		bool bFoundAttackSpeed = false;
		const float AttackSpeed = UAbilitySystemBlueprintLibrary::GetFloatAttribute(GetAvatarActorFromActorInfo(), UAssassinsCombatSet::GetAttackSpeedAttribute(), bFoundAttackSpeed);
		if (bFoundAttackSpeed && (AttackSpeed > 0.0f))
		{
			const double AttackSpeedInversed = 1.0 / AttackSpeed;
			MontageRate = MontageToPlay->GetPlayLength() / AttackSpeedInversed;
		}
	}

	if (MontageRate > 0.0)
	{
		HitEventTime = HitEventTime / MontageRate;
	}
}

bool UGA_Attack::ShouldKeepFollowingTarget() const
{
	const UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponentFromActorInfo();
	if (AssassinsASC == nullptr)
	{
		return false;
	}

	return AssassinsASC->HasAnyMatchingGameplayTags(FGameplayTagContainer::CreateFromArray(ActionBlockingTags));
}

TSubclassOf<UGameplayEffect> UGA_Attack::GetAttackEffectClass() const
{
	return AttackEffectClass;
}

void UGA_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled)
	{
		CancelActivateAttack();
	}

	IsMontageEnded = false;
	ResetTargetState();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
