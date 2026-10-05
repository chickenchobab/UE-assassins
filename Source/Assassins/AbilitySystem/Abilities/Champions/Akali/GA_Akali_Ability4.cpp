#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Ability4.h"

#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsChampion.h"
#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"
#include "Components/CapsuleComponent.h"
#include "NativeGameplayTags.h"
#include "Player/AssassinsPlayerController.h"

namespace AkaliUltimate
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ULTIMATE_RECAST, "Event.Champion.Akali.UltimateRecast");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY4_1, "Status.Combo.Ability4.1");

	// 서버가 소유 클라이언트에게 돌진의 대상 명중을 알린다. GameCustom1은 UGA_UnitTargeted의 도착이 쓴다.
	static constexpr EAbilityCustomReplicatedEvent HitNotifyEvent = EAbilityCustomReplicatedEvent::GameCustom2;

	static constexpr EAbilityCustomReplicatedEvent HitPredictionEvent = EAbilityCustomReplicatedEvent::GameCustom3;

	// 대상이 돌진 경로에서 캡슐 범위를 벗어나도 서버가 클라이언트가 본 명중을 받아들이는 거리. 명중 소식이 오가는 동안 대상이 걷는 정도다.
	static constexpr double HitPredictionTolerance = 150.0;

	static constexpr double DirectionTolerance = 1.e-4;
};

UGA_Akali_Ability4::UGA_Akali_Ability4(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Akali_Ability4::PrePlayMontage()
{
	Super::PrePlayMontage();

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsActive() || (AvatarActor == nullptr))
	{
		return;
	}

	DashStartLocation = AvatarActor->GetActorLocation();
	const FVector StartToTarget = AbilityTargetActor->GetActorLocation() - DashStartLocation;
	ToTarget = FVector(StartToTarget.X, StartToTarget.Y, 0.0);

	AddTagToAvatar(AkaliUltimate::TAG_STATUS_COMBO_ABILITY4_1);

	const FVector DashLocation = DashStartLocation + (ToTarget.GetSafeNormal2D(AkaliUltimate::DirectionTolerance) * (DashDistance + ToTarget.Size2D()));

	UAbilityTask_DashTo* DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, DashLocation, DashSpeed, 0.0f,
		ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.0f);
	DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->ReadyForActivation();
}

FGameplayTagContainer UGA_Akali_Ability4::GetPendingCastTags() const
{
	// 돌진이 대상에 닿을 때까지 붙드는 태그(PrePlayMontage). E·W는 명중을 기다린다.
	// Status.Dashing 대신 Q와 이동 클릭도 막는다. Status.Dashing은 돌진이 더하지 않고 설정해서 미리 붙들 수 없고,
	// 없으면 그사이 누른 Q가 시작돼 돌진에 끊긴다.
	FGameplayTagContainer PendingCastTags(AkaliUltimate::TAG_STATUS_COMBO_ABILITY4_1);
	PendingCastTags.AddTag(AssassinsGameplayTags::Status_Channeling);
	return PendingCastTags;
}

void UGA_Akali_Ability4::PostPlayMontage()
{
	// 바로 맞는 대상이라도 명중 몽타주가 돌진 몽타주 위에 재생되도록, 돌진 몽타주가 시작된 뒤에 캡슐을 켠다. 이미 겹친 적도 센다.
	AcquireCapsule();

	if (K2_HasAuthority())
	{
		if (!IsLocallyControlled())
		{
			UAbilityTask_WaitReplicatedEvent* HitPredictionTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, AkaliUltimate::HitPredictionEvent);
			HitPredictionTask->OnEvent.AddDynamic(this, &ThisClass::OnClientPredictedHit);
			HitPredictionTask->ReadyForActivation();
		}
		return;
	}

	UAbilityTask_WaitReplicatedEvent* HitNotifyTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, AkaliUltimate::HitNotifyEvent);
	HitNotifyTask->OnEvent.AddDynamic(this, &ThisClass::ClientGetNotifiedOfHitEvent);
	HitNotifyTask->ReadyForActivation();
}

void UGA_Akali_Ability4::AcquireCapsule()
{
	UAssassinsChampionSkillState_Akali* AkaliState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Akali>(GetAvatarActorFromActorInfo());
	if (AkaliState == nullptr)
	{
		return;
	}

	AbilityCapsule = AkaliState->AcquireUltimateCapsule(this);
	if (AbilityCapsule == nullptr)
	{
		return;
	}

	CapsuleRadius = AbilityCapsule->GetScaledCapsuleRadius();
	AbilityCapsule->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnTargetHit);

	TArray<AActor*> OverlappingActors;
	AbilityCapsule->GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		HandleOverlappedActor(OverlappingActor);
	}
}

void UGA_Akali_Ability4::ReleaseCapsule()
{
	if (AbilityCapsule == nullptr)
	{
		return;
	}

	AbilityCapsule->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnTargetHit);
	AbilityCapsule = nullptr;

	if (UAssassinsChampionSkillState_Akali* AkaliState = AAssassinsChampion::FindSkillState<UAssassinsChampionSkillState_Akali>(GetAvatarActorFromActorInfo()))
	{
		AkaliState->ReleaseUltimateCapsule(this);
	}
}

void UGA_Akali_Ability4::OnTargetHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HandleOverlappedActor(OtherActor);
}

void UGA_Akali_Ability4::HandleOverlappedActor(AActor* TargetActor)
{
	if (!IsActive() || !IsValidEnemy(TargetActor) || OverlappedActors.Contains(TargetActor))
	{
		return;
	}

	// 소유 클라이언트는 대상 명중만 미리 본다. 명중 판정은 서버가 한다.
	if (!K2_HasAuthority())
	{
		if (TargetActor == AbilityTargetActor)
		{
			PredictTargetHit();
		}
		return;
	}

	OverlappedActors.Add(TargetActor);
	ApplyGameplayEffectSpecToTargetActor(MakeEffectSpecHandle(DamageEffectClass), TargetActor);

	HasHitAbilityTarget = HasHitAbilityTarget || (TargetActor == AbilityTargetActor);
	if (!HasHitAbilityTarget || HasServerNotifiedClientHitEvent)
	{
		return;
	}

	HasServerNotifiedClientHitEvent = true;
	if (!IsLocallyControlled())
	{
		ClientSetReplicatedEvent(AkaliUltimate::HitNotifyEvent);
	}

	PlayHitDash();
}

void UGA_Akali_Ability4::PredictTargetHit()
{
	if (HasHitAbilityTarget)
	{
		return;
	}

	HasHitAbilityTarget = true;

	// 서버 왕복을 기다리면 그사이 돌진이 대상을 지나쳐, 명중 직후 E가 시작 방향으로 뒤집힌다(지연이 있으면 더하다).
	// 서버는 소식을 받는 즉시 명중을 처리해, 명중으로 클라이언트가 시전한 E·W를 자기 돌진이 아직 대상에 닿지 않았다고 거절하지 않게 한다.
	SendPredictedEventToServer(AkaliUltimate::HitPredictionEvent, [this]() { PlayHitDash(); });
}

void UGA_Akali_Ability4::ClientGetNotifiedOfHitEvent()
{
	if (HasHitAbilityTarget)
	{
		return;
	}

	HasHitAbilityTarget = true;
	PlayHitDash();
}

void UGA_Akali_Ability4::OnClientPredictedHit()
{
	if (HasHitAbilityTarget || !IsTargetAlongDash())
	{
		return;
	}

	HandleOverlappedActor(AbilityTargetActor);
}

bool UGA_Akali_Ability4::IsTargetAlongDash() const
{
	if (!IsValid(AbilityTargetActor))
	{
		return false;
	}

	const FVector DashDirection = ToTarget.GetSafeNormal2D(AkaliUltimate::DirectionTolerance);
	const FVector DashStart(DashStartLocation.X, DashStartLocation.Y, 0.0);
	const FVector DashEnd = DashStart + (DashDirection * (DashDistance + ToTarget.Size2D()));

	const FVector TargetLocation = AbilityTargetActor->GetActorLocation();
	const double TargetDistance = FMath::PointDistToSegment(FVector(TargetLocation.X, TargetLocation.Y, 0.0), DashStart, DashEnd);
	return TargetDistance <= (CapsuleRadius + AbilityTargetActor->GetSimpleCollisionRadius() + AkaliUltimate::HitPredictionTolerance);
}

void UGA_Akali_Ability4::PlayHitDash()
{
	RemoveTagFromAvatar(AkaliUltimate::TAG_STATUS_COMBO_ABILITY4_1);

	UAbilityTask_NetworkSyncPoint* SyncTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyServerWait);
	SyncTask->OnSync.AddDynamic(this, &ThisClass::OnHitDashSynced);
	SyncTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnHitDashSynced()
{
	UAbilityTask_PlayMontageAndWait* HitMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, HitDashMontage, 1.0f, NAME_None, true);
	HitMontageTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnDashEnded()
{
	ReleaseCapsule();

	if (IsLocallyControlled())
	{
		WaitAndHandleRecast();
	}
}

void UGA_Akali_Ability4::WaitAndHandleRecast()
{
	UAbilityTask_WaitDelay* PermissionTask = UAbilityTask_WaitDelay::WaitDelay(this, RecastPermissionTime);
	PermissionTask->OnFinish.AddDynamic(this, &ThisClass::OnRecastPermissionEnded);
	PermissionTask->ReadyForActivation();

	UAbilityTask_WaitDelay* RecastDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, RecastDelay);
	RecastDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnRecastAllowed);
	RecastDelayTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnRecastPermissionEnded()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability4::OnRecastAllowed()
{
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastInputReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnRecastInputReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, false);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastInputPressed);
	PressTask->ReadyForActivation();
}

void UGA_Akali_Ability4::OnRecastInputPressed(float TimeWaited)
{
	// 다른 어빌리티 시전 중(Status.Channeling)에 누르면 재시전이 시전을 끊으므로(Q 취소) 거절하고 다음 입력을 기다린다.
	// 소유 클라이언트만 판단하고, 서버는 보내온 재시전만 본다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(AssassinsGameplayTags::Status_Channeling))
	{
		OnRecastAllowed();
		return;
	}

	Recast();
}

void UGA_Akali_Ability4::Recast()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC == nullptr)
	{
		return;
	}

	// 재시전은 이 이벤트로 시작하는 별도 어빌리티다. 갈 곳을 컨텍스트에서 읽고, 이벤트 데이터가 활성화와 함께 가서 서버에서도 읽는다.
	RecastEventContext = ASC->MakeEffectContext();
	if (AAssassinsPlayerController* AssassinsPlayerController = GetAssassinsPlayerControllerFromActorInfo())
	{
		FHitResult CursorHitResult;
		AssassinsPlayerController->GetHitResultUnderCursorByChannel(TraceTypeQuery1, true, CursorHitResult);
		RecastEventContext.AddHitResult(CursorHitResult, false);
	}

	FGameplayEventData Payload;
	Payload.ContextHandle = RecastEventContext;
	SendGameplayEvent(AkaliUltimate::TAG_EVENT_ULTIMATE_RECAST, Payload);

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability4::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ReleaseCapsule();

	OverlappedActors.Empty();
	HasHitAbilityTarget = false;
	HasServerNotifiedClientHitEvent = false;
	RecastEventContext = FGameplayEffectContextHandle();
	CapsuleRadius = 0.0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
