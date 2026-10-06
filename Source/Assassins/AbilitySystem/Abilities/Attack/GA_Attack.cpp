// Fill out your copyright notice in the Description page of Project Settings.

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
	// Asks the ability that keeps attacking to take a target.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ACTIVATE_ATTACK, "Event.ActivateAttack");

	// The ability that keeps attacking, and the status it gives the avatar while it does.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ACTIVATE_ATTACK, "Ability.ActivateAttack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ACTIVATING_ATTACK, "Status.Ability.ActivatingAttack");

	// Set on the avatar from the start of the montage until the attack lands.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ATTACK, "Status.Combo.Attack");
};

namespace
{
	// What UAbilitySystemBlueprintLibrary::IsActiveGameplayEffectHandleActive does, which is not exported to this module.
	bool IsGameplayEffectHandleActive(const FActiveGameplayEffectHandle& Handle)
	{
		const UAbilitySystemComponent* OwningASC = Handle.GetOwningAbilitySystemComponent();
		return Handle.IsValid() && OwningASC && (OwningASC->GetActiveGameplayEffect(Handle) != nullptr);
	}
}

UGA_Attack::UGA_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The basic attack is the only thing meant to hit a structure.
	bCanTargetStructure = true;
}

void UGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UpdateTargetChasingCondition();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Only the one who clicks can pick another target, and only while the attack is still running.
	if (IsActive() && IsLocallyControlled())
	{
		GetReadyToTargetChange();
	}
}

void UGA_Attack::UpdateTargetChasingCondition()
{
	// The server owns the movement, so it is the one deciding whether the chase goes on.
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
	// A new target can only have been picked after the click was released and pressed again.
	UAbilityTask_WaitInputRelease* WaitReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased*/ true);
	WaitReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
	WaitReleaseTask->ReadyForActivation();
}

void UGA_Attack::OnInputReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* WaitPressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, /*bTestAlreadyPressed*/ true);
	WaitPressTask->OnPress.AddDynamic(this, &ThisClass::OnInputPressed);
	WaitPressTask->ReadyForActivation();
}

void UGA_Attack::OnInputPressed(float TimeWaited)
{
	AActor* CursorTarget = GetCurrentCursorTarget();

	// Units and the enemy base are worth attacking, anything else leaves the attack on its target.
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

	// The ability that keeps attacking is cancelled just above: the event only starts it again on the next tick.
	PendingAttackTarget = NewTarget;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::SendPendingAttackEvent);
	}
}

void UGA_Attack::SendPendingAttackEvent()
{
	SendActivateAttackEvent(PendingAttackTarget);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Attack::SendActivateAttackEvent(AActor* TargetActor)
{
	// The ability that keeps attacking runs where the avatar is controlled(local only): the engine starts it from the
	// event there only(UAbilitySystemComponent::HasNetworkAuthorityToActivateTriggeredAbility). Sent anywhere else, the
	// event starts nothing. The melee attack sent it there only and the ranged one everywhere, to the same effect.
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
	// Not calling Super: the attack is used to reach the target as well, so it never commits.
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
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	UAssassinsTargetChasingComponent* ChasingComponent = GetTargetChasingComponentFromController();
	if (!IsValid(ChasingComponent))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	// The avatar goes on following the target, so the attack waits for the next arrival.
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

	// The owning client is told the avatar arrived, and both sides start the montage together.
	ClientSetReplicatedEvent(EAbilityCustomReplicatedEvent::GameCustom1);
	WaitForNetSync();
}

void UGA_Attack::PlayMontage()
{
	PrePlayMontage();

	// PrePlayMontage ends the ability when the target is gone.
	if (!IsActive())
	{
		return;
	}

	// Picked a second time on purpose: the cooldown PrePlayMontage applied is what the montage is fitted in now.
	SetMontageToPlay();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, static_cast<float>(MontageRate), NAME_None, /*bStopWhenAbilityEnds*/ true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	// Blending out counts as done too, so that the next attack follows this one without a gap.
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCanceled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCanceled);
	MontageTask->ReadyForActivation();

	// A montage that fails to play is cancelled right inside ReadyForActivation, which may already have ended the
	// ability. The hit timer must not outlive it: EndAbility has cleared the timers before this point.
	if (IsActive())
	{
		PostPlayMontage();
	}
}

void UGA_Attack::PrePlayMontage()
{
	// The cooldown is applied before the montage: what is left of it is what the montage is made to fit in.
	AttackCooldownActiveHandle = BP_ApplyGameplayEffectToOwner(AttackCooldownEffectClass, /*GameplayEffectLevel*/ 1, /*Stacks*/ 1);
	AddTagToAvatar(AttackTags::TAG_STATUS_COMBO_ATTACK);

	Super::PrePlayMontage();
}

void UGA_Attack::PostPlayMontage()
{
	// The attack lands at the timing that came with the montage.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AttackHitTimerHandle, this, &ThisClass::OnHitTimerElapsed, static_cast<float>(HitEventTime), /*bLoop*/ false);
	}
}

void UGA_Attack::OnHitTimerElapsed()
{
	// An ended ability must not land a hit: the client would pass the sync point on its own and deal the damage.
	if (!IsActive())
	{
		return;
	}

	// The server waits for the client to reach the hit as well, so that both sides agree on it.
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

	// The attack only ends by itself while the ability that keeps attacking is there to start the next one.
	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter && AvatarCharacter->HasGameplayTag(AttackTags::TAG_STATUS_ABILITY_ACTIVATING_ATTACK))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
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
		// An attack is already on cooldown: the montage lasts exactly as long as what is left of it.
		const float RemainingCooldown = UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectRemainingDuration(this, AttackCooldownActiveHandle);
		if (RemainingCooldown > 0.0f)
		{
			MontageRate = MontageToPlay->GetPlayLength() / RemainingCooldown;
		}
	}
	else
	{
		// The first attack has no cooldown to go by, so the attack speed says how long the montage lasts.
		bool bFoundAttackSpeed = false;
		const float AttackSpeed = UAbilitySystemBlueprintLibrary::GetFloatAttribute(GetAvatarActorFromActorInfo(), UAssassinsCombatSet::GetAttackSpeedAttribute(), bFoundAttackSpeed);
		if (bFoundAttackSpeed && (AttackSpeed > 0.0f))
		{
			const double AttackSpeedInversed = 1.0 / AttackSpeed;
			MontageRate = MontageToPlay->GetPlayLength() / AttackSpeedInversed;
		}
	}

	// The hit follows the rate the montage ends up playing at.
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

	// Something is keeping the avatar from acting, so it has to go on following the target.
	return AssassinsASC->HasAnyMatchingGameplayTags(FGameplayTagContainer::CreateFromArray(ActionBlockingTags));
}

TSubclassOf<UGameplayEffect> UGA_Attack::GetAttackEffectClass() const
{
	return AttackEffectClass;
}

void UGA_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Being cancelled means the avatar was told to do something else, so it stops attacking altogether.
	if (bWasCancelled)
	{
		CancelActivateAttack();
	}

	IsMontageEnded = false;
	ResetTargetState();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
