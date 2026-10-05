// Fill out your copyright notice in the Description page of Project Settings.

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

	// The shroud drops a moment after the cast, where the cue shows it falling.
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
	// Faces the cursor.
	Super::PrePlayMontage();

	// Turning is instant again, whatever an earlier ability left.
	if (AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		AvatarCharacter->ResetRotationRate();
	}

	if (!K2_HasAuthority())
	{
		return;
	}

	// Any other ability, or an attack, breaks the stealth. The server sees every ability that runs on it, so there is
	// nothing to hear from the client: abilities that only run on the client do not break it anywhere.
	UAbilityTask_WaitAbilityActivate* ActivateTask = UAbilityTask_WaitAbilityActivate::WaitForAbilityActivate(
		this, FGameplayTag(), AkaliShroud::TAG_ABILITY_ATTACK, /*IncludeTriggeredAbilities*/ true, /*TriggerOnce*/ false);
	ActivateTask->OnActivate.AddDynamic(this, &ThisClass::OnOtherAbilityActivated);
	ActivateTask->ReadyForActivation();

	UAbilityTask_WaitGameplayTagAdded* AttackTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
		this, AkaliShroud::TAG_STATUS_ABILITY_ATTACKING, /*InOptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ false);
	AttackTask->Added.AddDynamic(this, &ThisClass::OnAttackStarted);
	AttackTask->ReadyForActivation();
}

void UGA_Akali_Ability2::PostPlayMontage()
{
	FGameplayCueParameters CueParameters;
	CueParameters.Location = CursorLocationClamped + FVector(0.0, 0.0, AkaliShroud::CueHeight);
	K2_AddGameplayCueWithParams(AkaliShroud::TAG_GAMEPLAYCUE_SHROUD, CueParameters, /*bRemoveOnAbilityEnd*/ true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ShroudSpawnTimerHandle, this, &ThisClass::SpawnShroud, AkaliShroud::ShroudSpawnDelay, /*bLoop*/ false);
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

	// Both sides spawn one: the shroud keeps the owning client's copy of the server's one hidden.
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

	// The ability lasts as long as the shroud.
	SpawnedShroud->OnEndPlay.AddDynamic(this, &ThisClass::OnShroudEndPlay);

	// Whether Akali is in the ring is checked every frame, but the stealth only changes when it has to.
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
		// Applied once: the effect is infinite and does not stack, every extra one would stay.
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
	StealthEffectHandle = BP_ApplyGameplayEffectToOwner(StealthEffectClass, /*GameplayEffectLevel*/ 1, /*Stacks*/ 1);
}

void UGA_Akali_Ability2::FinishStealth()
{
	if (StealthEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(StealthEffectHandle, /*StacksToRemove*/ 1);
	}
	StealthEffectHandle.Invalidate();
}

void UGA_Akali_Ability2::BreakStealth()
{
	FinishStealth();
	BP_ApplyGameplayEffectToOwner(DisableStealthEffectClass, /*GameplayEffectLevel*/ 1, /*Stacks*/ 1);
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
	// Only this activation's shroud counts.
	if (Actor != SpawnedShroud)
	{
		return;
	}

	K2_RemoveGameplayCue(AkaliShroud::TAG_GAMEPLAYCUE_SHROUD);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Akali_Ability2::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsValid(SpawnedShroud))
	{
		// The shroud outlives the ability: its end must not reach the next activation.
		SpawnedShroud->OnEndPlay.RemoveDynamic(this, &ThisClass::OnShroudEndPlay);
		SpawnedShroud->SetLifeSpan(0.1f);
	}
	SpawnedShroud = nullptr;

	FinishStealth();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
