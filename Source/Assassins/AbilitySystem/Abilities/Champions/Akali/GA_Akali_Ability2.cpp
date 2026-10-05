#include "AbilitySystem/Abilities/Champions/Akali/GA_Akali_Ability2.h"

#include "Abilities/Tasks/AbilityTask_WaitAbilityActivate.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/Tasks/AbilityTask_CustomizeTickTask.h"
#include "AbilitySystemComponent.h"
#include "Character/AssassinsCharacter.h"
#include "Character/Champions/Akali/AssassinsShroud.h"
#include "Engine/CurveTable.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

namespace AkaliShroud
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GAMEPLAYCUE_SHROUD, "GameplayCue.Champion.Akali.Shroud");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ATTACK, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ATTACKING, "Status.Ability.Attacking");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_STEALTH_DISABLED, "Status.Champion.Akali.StealthDisabled");

	static constexpr float ShroudSpawnDelay = 0.1f;
	static constexpr double CueHeight = 200.0;
};

UGA_Akali_Ability2::UGA_Akali_Ability2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ShroudDurationRow = TEXT("Ability2.ShroudDuration");
}

void UGA_Akali_Ability2::PrePlayMontage()
{
	Super::PrePlayMontage();

	if (AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		AvatarCharacter->ResetRotationRate();
	}

	if (!K2_HasAuthority())
	{
		return;
	}

	// 다른 어빌리티나 공격이 은신을 깬다. 서버는 자기에서 도는 모든 어빌리티를 보므로 클라이언트에게 들을 것이 없다.
	// 클라이언트에서만 도는 어빌리티는 어디서도 은신을 깨지 않는다.
	UAbilityTask_WaitAbilityActivate* ActivateTask = UAbilityTask_WaitAbilityActivate::WaitForAbilityActivate(
		this, FGameplayTag(), AkaliShroud::TAG_ABILITY_ATTACK, true, false);
	ActivateTask->OnActivate.AddDynamic(this, &ThisClass::OnOtherAbilityActivated);
	ActivateTask->ReadyForActivation();

	UAbilityTask_WaitGameplayTagAdded* AttackTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
		this, AkaliShroud::TAG_STATUS_ABILITY_ATTACKING, nullptr, false);
	AttackTask->Added.AddDynamic(this, &ThisClass::OnAttackStarted);
	AttackTask->ReadyForActivation();
}

void UGA_Akali_Ability2::PostPlayMontage()
{
	FGameplayCueParameters CueParameters;
	CueParameters.Location = CursorLocationClamped + FVector(0.0, 0.0, AkaliShroud::CueHeight);
	K2_AddGameplayCueWithParams(AkaliShroud::TAG_GAMEPLAYCUE_SHROUD, CueParameters, true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ShroudSpawnTimerHandle, this, &ThisClass::SpawnShroud, AkaliShroud::ShroudSpawnDelay, false);
	}
}

void UGA_Akali_Ability2::SpawnShroud()
{
	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	UWorld* World = GetWorld();
	if ((AvatarCharacter == nullptr) || (World == nullptr) || !ShroudClass)
	{
		return;
	}

	// 양쪽 모두 장막을 만든다. 소유 클라이언트에서는 장막이 서버 장막의 복제본을 숨긴다.
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Owner = AvatarCharacter;
	SpawnParameters.Instigator = AvatarCharacter;

	const FTransform SpawnTransform(FRotator::ZeroRotator, FVector(CursorLocationClamped.X, CursorLocationClamped.Y, 0.0));
	SpawnedShroud = World->SpawnActor<AAssassinsShroud>(ShroudClass, SpawnTransform, SpawnParameters);
	if (SpawnedShroud != nullptr)
	{
		InitShroud();
	}
}

void UGA_Akali_Ability2::InitShroud()
{
	const float ShroudDuration = EvaluateCurveTableRowByAbilityLevel(AbilityMagnitudeTable, ShroudDurationRow, TEXT("Akali Ability2 shroud duration"));
	SpawnedShroud->SetLifeSpan(ShroudDuration);

	if (!K2_HasAuthority())
	{
		return;
	}

	SpawnedShroud->OnEndPlay.AddDynamic(this, &ThisClass::OnShroudEndPlay);

	UAbilityTask_CustomizeTickTask* TickTask = UAbilityTask_CustomizeTickTask::CustomizeTickTask(this);
	TickTask->OnTickTask.AddDynamic(this, &ThisClass::OnStealthTick);
	TickTask->ReadyForActivation();

	UpdateStealth();
}

void UGA_Akali_Ability2::OnStealthTick(float DeltaTime)
{
	UpdateStealth();
}

void UGA_Akali_Ability2::UpdateStealth()
{
	bool bInsideRing = false;
	if (IsValid(SpawnedShroud))
	{
		SpawnedShroud->IsActorInStealthRadius(GetAvatarActorFromActorInfo(), bInsideRing);
	}

	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	const bool bStealthDisabled = AvatarCharacter && AvatarCharacter->HasGameplayTag(AkaliShroud::TAG_STATUS_STEALTH_DISABLED);

	if (bInsideRing && !bStealthDisabled)
	{
		if (!IsStealthActive())
		{
			ApplyStealth();
		}
	}
	else if (!bInsideRing)
	{
		FinishStealth();
	}
}

bool UGA_Akali_Ability2::IsStealthActive() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return StealthEffectHandle.IsValid() && ASC && (ASC->GetActiveGameplayEffect(StealthEffectHandle) != nullptr);
}

void UGA_Akali_Ability2::ApplyStealth()
{
	StealthEffectHandle = BP_ApplyGameplayEffectToOwner(StealthEffectClass, 1, 1);
}

void UGA_Akali_Ability2::FinishStealth()
{
	if (StealthEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(StealthEffectHandle, 1);
	}
	StealthEffectHandle.Invalidate();
}

void UGA_Akali_Ability2::BreakStealth()
{
	FinishStealth();
	BP_ApplyGameplayEffectToOwner(DisableStealthEffectClass, 1, 1);
}

void UGA_Akali_Ability2::OnOtherAbilityActivated(UGameplayAbility* ActivatedAbility)
{
	if (ActivatedAbility != this)
	{
		BreakStealth();
	}
}

void UGA_Akali_Ability2::OnAttackStarted()
{
	BreakStealth();
}

void UGA_Akali_Ability2::OnShroudEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (Actor != SpawnedShroud)
	{
		return;
	}

	K2_RemoveGameplayCue(AkaliShroud::TAG_GAMEPLAYCUE_SHROUD);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Akali_Ability2::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsValid(SpawnedShroud))
	{
		// 장막이 어빌리티보다 오래 살아서, 장막의 종료가 다음 활성화에 닿지 않게 푼다.
		SpawnedShroud->OnEndPlay.RemoveDynamic(this, &ThisClass::OnShroudEndPlay);
		SpawnedShroud->SetLifeSpan(0.1f);
	}
	SpawnedShroud = nullptr;

	FinishStealth();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
