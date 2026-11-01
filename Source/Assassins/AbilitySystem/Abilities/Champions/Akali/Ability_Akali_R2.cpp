#include "AbilitySystem/Abilities/Champions/Akali/Ability_Akali_R2.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/AssassinsInterframeHitCheckSubsystem.h"
#include "AbilitySystem/Tasks/AbilityTask_ApplyRootMotionDash.h"
#include "Animation/AssassinsAnimInstance.h"
#include "AssassinsLogCategories.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/Champions/Akali/AssassinsChampionSkillState_Akali.h"
#include "Engine/CurveTable.h"
#include "GameFramework/Character.h"
#include "NativeGameplayTags.h"

namespace AkaliUltimateRecast
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ANIMNOTIFY_PAUSE_MONTAGE, "Event.AnimNotify.PauseMontage");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_ULTIMATE_RECAST_DASH, "GameplayCue.Champion.Akali.UltimateRecastDash");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_ULTIMATE_RECAST_HIT, "GameplayCue.Champion.Akali.UltimateRecastHit");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SETBYCALLER_DAMAGE_MULTIPLICATION, "SetByCaller.Akali.DamageMultiplication");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_COMBO_ABILITY4_2, "Status.Combo.R.2");
};

UAbility_Akali_R2::UAbility_Akali_R2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DamagePerMissingHealthRow = TEXT("TotalDamagePercent");
}

void UAbility_Akali_R2::PrePlayMontage()
{
	Super::PrePlayMontage();

	if (K2_HasAuthority())
	{
		// 이전 돌진의 명중 목록은 끝날 때 비우지 않고 여기까지 둔다. 늦게 판정된 명중도 그 돌진에 한 번만 센다(HandleCheckedHit).
		++ActivationNumber;
		HitActors.Empty();
		HasPlayedHitEffect = false;
		DamageSpecHandle = FGameplayEffectSpecHandle();

		// 모양 안에 이미 선 적은 첫 이동이 시작하면서 찾는다.
		BeginHits();
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsActive() || (AvatarActor == nullptr))
	{
		return;
	}

	AddTagToAvatar(AkaliUltimateRecast::TAG_STATUS_COMBO_ABILITY4_2);

	const FVector DashLocation = AvatarActor->GetActorLocation() + (CursorLookAtRotation.Vector() * AbilityRange);

	UAbilityTask_DashTo* DashTask = UAbilityTask_DashTo::DashTo(this, NAME_None, DashLocation, DashSpeed, 0.0f,
		ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.0f);
	DashTask->OnCancelled.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->OnFinished.AddDynamic(this, &ThisClass::OnDashEnded);
	DashTask->ReadyForActivation();

	if (!IsActive())
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.Normal = CursorLookAtRotation.Vector();
	K2_ExecuteGameplayCueWithParams(AkaliUltimateRecast::TAG_GAMEPLAYCUE_ULTIMATE_RECAST_DASH, CueParameters);
}

void UAbility_Akali_R2::PostPlayMontage()
{
	UAbilityTask_WaitGameplayEvent* PauseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, AkaliUltimateRecast::TAG_EVENT_ANIMNOTIFY_PAUSE_MONTAGE, nullptr, true, true);
	PauseTask->EventReceived.AddDynamic(this, &ThisClass::OnPauseMontageNotify);
	PauseTask->ReadyForActivation();
}

void UAbility_Akali_R2::OnPauseMontageNotify(FGameplayEventData Payload)
{
	if (UAssassinsAnimInstance* AnimInstance = GetAssassinsAnimInstanceFromActorInfo())
	{
		AnimInstance->Montage_Pause(MontageToPlay);
	}
}

void UAbility_Akali_R2::OnDashEnded()
{
	RemoveTagFromAvatar(AkaliUltimateRecast::TAG_STATUS_COMBO_ABILITY4_2);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UAbility_Akali_R2::BeginHits()
{
	ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAssassinsInterframeHitCheckSubsystem* HitCheck = UAssassinsInterframeHitCheckSubsystem::Get(this);
	if ((AvatarCharacter == nullptr) || (HitCheck == nullptr))
	{
		return;
	}

	const TWeakObjectPtr<ThisClass> WeakThis(this);
	const int32 CheckedActivation = ActivationNumber;
	HitShapeHandle = HitCheck->BeginDashHitShape(AvatarCharacter, UAssassinsChampionSkillState_Akali::GetUltimateHitShape(),
		[WeakThis](AActor* Target)
		{
			const ThisClass* Self = WeakThis.Get();
			return (Self != nullptr) && Self->IsValidEnemy(Target) && !Self->HitActors.Contains(Target);
		},
		[WeakThis, CheckedActivation](AActor* Target)
		{
			if (ThisClass* Self = WeakThis.Get())
			{
				Self->HandleCheckedHit(Target, CheckedActivation);
			}
		});
}

void UAbility_Akali_R2::EndHits()
{
	if (HitShapeHandle == 0)
	{
		return;
	}

	if (UAssassinsInterframeHitCheckSubsystem* HitCheck = UAssassinsInterframeHitCheckSubsystem::Get(this))
	{
		HitCheck->EndDashHitShape(HitShapeHandle);
	}
	HitShapeHandle = 0;
}

void UAbility_Akali_R2::HandleCheckedHit(AActor* Target, int32 CheckedActivation)
{
	// 대상의 이동을 기다린 판정은 돌진과 재시전이 끝난 뒤에 올 수 있다. 명중은 그 돌진 중에 일어났으므로 그 돌진에 세고,
	// 뒤의 돌진에는 세지 않는다(그 돌진은 자기 명중이 따로 있다).
	if (CheckedActivation != ActivationNumber)
	{
		UE_LOG(LogAssassinsAbilitySystem, Warning, TEXT("%s: the hit on %s was checked after the next recast started, and is dropped."),
			*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(Target));
		return;
	}

	UE_CLOG(!IsActive(), LogAssassinsAbilitySystem, Verbose, TEXT("%s: the hit on %s was checked after the recast ended."),
		*GetNameSafe(GetAvatarActorFromActorInfo()), *GetNameSafe(Target));
	ApplyHit(Target);
}

void UAbility_Akali_R2::ApplyHit(AActor* Target)
{
	if (!IsValidEnemy(Target) || HitActors.Contains(Target))
	{
		return;
	}

	HitActors.Add(Target);

	DamageSpecHandle = MakeEffectSpecHandle(DamageEffectClass);
	if (FGameplayEffectSpec* DamageSpec = DamageSpecHandle.Data.Get())
	{
		DamageSpec->SetSetByCallerMagnitude(AkaliUltimateRecast::TAG_SETBYCALLER_DAMAGE_MULTIPLICATION, CalculateDamageIncrease(Target));
	}
	ApplyGameplayEffectSpecToTargetActor(DamageSpecHandle, Target);

	if (!HasPlayedHitEffect)
	{
		K2_ExecuteGameplayCue(AkaliUltimateRecast::TAG_GAMEPLAYCUE_ULTIMATE_RECAST_HIT, FGameplayEffectContextHandle());
		HasPlayedHitEffect = true;
	}
}

float UAbility_Akali_R2::CalculateDamageIncrease(AActor* Actor) const
{
	const UAssassinsHealthComponent* HealthComponent = UAssassinsHealthComponent::FindHealthComponent(Actor);
	if ((HealthComponent == nullptr) || (DamagePerMissingHealthTable == nullptr))
	{
		return 0.0f;
	}

	const FRealCurve* Curve = DamagePerMissingHealthTable->FindCurve(DamagePerMissingHealthRow, TEXT("Akali recast damage per missing health"));
	return Curve ? Curve->Eval(1.0f - HealthComponent->GetHealthNormalized()) : 0.0f;
}

void UAbility_Akali_R2::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	EndHits();

	// 끝난 뒤 오는 판정을 위해 명중 목록을 남긴다. 다음 돌진에서 PrePlayMontage가 비운다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
