#include "AbilitySystem/Abilities/Champions/Leblanc/GA_Leblanc_Ability2.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitRecastPress.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AssassinsGameplayTags.h"
#include "Character/Champions/Leblanc/AssassinsChampionSkillState_Leblanc.h"
#include "CollisionQueryParams.h"
#include "GameFramework/RootMotionSource.h"
#include "NativeGameplayTags.h"

namespace LeblancDistortion
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ABILITY4, "Ability.Ability4");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_MIMIC_ABILITY2, "Event.Champion.Leblanc.Mimic.Ability2");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ULTIMATE_RECAST, "Event.Champion.Leblanc.UltimateRecast");

	static constexpr EAbilityCustomReplicatedEvent ReturnEvent = EAbilityCustomReplicatedEvent::GameCustom1;

	static constexpr double DamageTraceHeight = 300.0;
};

UGA_Leblanc_Ability2::UGA_Leblanc_Ability2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Leblanc_Ability2::PrePlayMontage()
{
	bDashFinished = false;
	bDashSynced = false;
	bRecastInputReleased = false;
	bReturnRequested = false;
	bReturnStarted = false;

	Super::PrePlayMontage();

	UAssassinsChampionSkillState_Leblanc::SetMimicAbilityOf(*this, LeblancDistortion::TAG_EVENT_MIMIC_ABILITY2);

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 커서를 본 뒤에 저장해, 돌아올 때 돌진한 방향을 보게 한다.
	ReturnLocation = AvatarActor->GetActorLocation();
	ReturnRotation = AvatarActor->GetActorRotation();

	FGameplayCueParameters CueParameters;
	CueParameters.Location = ReturnLocation;
	K2_AddGameplayCueWithParams(ReturnLocationCueTag, CueParameters, true);

	DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, CursorLocationClamped, DashSpeed, 0.0f,
		ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.0f);
	DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashCancelled);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::OnDashFinished);
	DashTask->ReadyForActivation();

	if (!IsActive())
	{
		return;
	}

	if (K2_HasAuthority() && !IsLocallyControlled())
	{
		WaitForRecast();
	}
	else if (IsLocallyControlled() && !IsMimic())
	{
		// 시전한 입력을 먼저 떼야 한다. 처음부터 지켜봐서 착지할 때 입력이 그 입력인지, 그 뒤 새로 누른 것인지 안다(OnDashSynced).
		WaitForRecastInputRelease();
	}
}

bool UGA_Leblanc_Ability2::IsMimic() const
{
	return GetAssetTags().HasTagExact(LeblancDistortion::TAG_ABILITY_ABILITY4);
}

void UGA_Leblanc_Ability2::OnDashFinished()
{
	HandleDashComplete(DashTask ? DashTask->GetCurrentTargetLocation() : CursorLocationClamped);
}

void UGA_Leblanc_Ability2::OnDashCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Leblanc_Ability2::HandleDashComplete(const FVector& EndLocation)
{
	if (bDashFinished)
	{
		return;
	}

	bDashFinished = true;
	DashEndLocation = EndLocation;

	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnDashSynced);
	SyncTask->ReadyForActivation();
}

void UGA_Leblanc_Ability2::OnDashSynced()
{
	bDashSynced = true;

	ApplyDashDamage();

	if (bReturnRequested)
	{
		StartReturn();
		return;
	}

	ReturnDurationTask = UAbilityTask_WaitDelay::WaitDelay(this, ReturnDuration);
	ReturnDurationTask->OnFinish.AddDynamic(this, &ThisClass::OnReturnDurationFinished);
	ReturnDurationTask->ReadyForActivation();

	if (!IsLocallyControlled())
	{
		return;
	}

	if (IsMimic())
	{
		WaitForRecast();
	}
	else if (bRecastInputReleased)
	{
		// 시전 입력을 뗀 뒤 다시 누른 상태면 그 입력으로 돌아간다. 돌진이 끝나는 프레임에 누른 입력은 끝보다 먼저 처리돼, 떼기를 기다리면 사라진다.
		WaitForReturnPress(true);
	}
}

void UGA_Leblanc_Ability2::ApplyDashDamage()
{
	DamageSpecHandle = MakeEffectSpecHandle(DamageEffectClass);

	for (const FHitResult& Hit : SweepForEnemies(DashEndLocation, EffectRadius, LeblancDistortion::DamageTraceHeight, FCollisionObjectQueryParams(ECC_Pawn)))
	{
		AActor* HitActor = Hit.GetActor();
		if (ActorsToIgnore.Contains(HitActor))
		{
			continue;
		}

		ApplyGameplayEffectSpecToTargetActor(DamageSpecHandle, HitActor);
		ActorsToIgnore.Add(HitActor);
	}
}

void UGA_Leblanc_Ability2::WaitForRecast()
{
	if (IsMimic())
	{
		if (IsLocallyControlled())
		{
			UAbilityTask_WaitGameplayEvent* RecastTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this, LeblancDistortion::TAG_EVENT_ULTIMATE_RECAST, nullptr, true, true);
			RecastTask->EventReceived.AddDynamic(this, &ThisClass::OnUltimateRecast);
			RecastTask->ReadyForActivation();
		}
		else
		{
			UAbilityTask_WaitReplicatedEvent* ReturnTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, LeblancDistortion::ReturnEvent);
			ReturnTask->OnEvent.AddDynamic(this, &ThisClass::OnReturnRequested);
			ReturnTask->ReadyForActivation();
		}
		return;
	}

	WaitForRecastInputRelease();
}

void UGA_Leblanc_Ability2::WaitForRecastInputRelease()
{
	// 입력 이벤트는 복제되어 서버도 같은 떼기·누르기를 받고, 누르기를 그 예측 키 안에서 처리한다.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastInputReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Leblanc_Ability2::OnRecastInputReleased(float TimeHeld)
{
	bRecastInputReleased = true;

	if (IsLocallyControlled() && !bDashSynced)
	{
		return;
	}

	WaitForReturnPress(false);
}

void UGA_Leblanc_Ability2::WaitForReturnPress(bool bTestAlreadyPressed)
{
	// 다른 어빌리티 시전 중 입력은 귀환하지 않는다(귀환이 시전을 끊는다). 소유 클라이언트가 받지 않으므로 서버에도 가지 않는다.
	UAbilityTask_WaitRecastPress* PressTask = UAbilityTask_WaitRecastPress::WaitRecastPress(this, FGameplayTagContainer(AssassinsGameplayTags::Status_Channeling), bTestAlreadyPressed);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastInputPressed);
	PressTask->ReadyForActivation();
}

void UGA_Leblanc_Ability2::OnRecastInputPressed()
{
	DoReturn();
}

void UGA_Leblanc_Ability2::OnUltimateRecast(FGameplayEventData Payload)
{
	SendPredictedEventToServer(LeblancDistortion::ReturnEvent, [this]() { DoReturn(); });
}

void UGA_Leblanc_Ability2::OnReturnRequested()
{
	DoReturn();
}

void UGA_Leblanc_Ability2::DoReturn()
{
	if (bReturnRequested || !IsActive())
	{
		return;
	}

	bReturnRequested = true;

	if (bDashSynced)
	{
		StartReturn();
		return;
	}

	// 서버만 여기 온다. 소유 클라이언트는 돌진 뒤에만 귀환하지만, 돌진의 마지막 이동이 이벤트보다 늦게 서버에 닿을 수 있다.
	// 돌진은 목표 지점에서 끝내고 귀환은 명중 뒤에 따른다(OnDashSynced). 돌진을 남겨 두면 클라이언트 이동이
	// 귀환 지점에 놓은 르블랑을 다시 목표로 끌고 간다.
	if (!bDashFinished)
	{
		FinishDashEarly();
	}
}

void UGA_Leblanc_Ability2::FinishDashEarly()
{
	FVector EndLocation = CursorLocationClamped;
	if (IsValid(DashTask))
	{
		EndLocation = DashTask->GetCurrentTargetLocation();

		// 취소가 아니다. 태스크를 끝내 루트 모션만 없앤다.
		DashTask->OnCancelled.RemoveDynamic(this, &ThisClass::OnDashCancelled);
		DashTask->EndTask();
	}

	HandleDashComplete(EndLocation);
}

void UGA_Leblanc_Ability2::StartReturn()
{
	if (bReturnStarted)
	{
		return;
	}

	bReturnStarted = true;

	if (IsValid(ReturnDurationTask))
	{
		ReturnDurationTask->EndTask();
	}
	ReturnDurationTask = nullptr;

	// 캐릭터 무브먼트로 순간이동한다. 서버는 소유 클라이언트의 이동에서 순간이동을 받는다.
	if (IsLocallyControlled())
	{
		SetAvatarLocationAndRotation(ReturnLocation, ReturnRotation);
	}

	K2_RemoveGameplayCue(ReturnLocationCueTag);

	UAbilityTask_PlayMontageAndWait* ReturnMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, ReturnMontage, 1.0f, NAME_None, true);
	ReturnMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnReturnMontageEnded);
	ReturnMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnReturnMontageEnded);
	ReturnMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnReturnMontageEnded);
	ReturnMontageTask->ReadyForActivation();
}

void UGA_Leblanc_Ability2::OnReturnMontageEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Leblanc_Ability2::OnReturnDurationFinished()
{
	K2_RemoveGameplayCue(ReturnLocationCueTag);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Leblanc_Ability2::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	DashTask = nullptr;
	ReturnDurationTask = nullptr;

	ActorsToIgnore.Empty();
	DamageSpecHandle = FGameplayEffectSpecHandle();
	DashEndLocation = FVector::ZeroVector;

	bDashFinished = false;
	bDashSynced = false;
	bRecastInputReleased = false;
	bReturnRequested = false;
	bReturnStarted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
