#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_Ability2.h"

#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Character/AssassinsCharacter.h"
#include "Character/Champions/Zed/AssassinsZedShadow.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

namespace ZedShadowAbility
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_SPAWNING_SHADOW, "Status.Champion.Zed.SpawningShadow");
};

UGA_Zed_Ability2::UGA_Zed_Ability2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Zed_Ability2::PrePlayMontage()
{
	// Super를 부르지 않는다. 이 스킬은 커서로 돌지 않는다.
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && CastParticle)
	{
		UGameplayStatics::SpawnEmitterAttached(CastParticle, AvatarActor->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			FVector::OneVector, EAttachLocation::SnapToTarget, true);
	}
}

void UGA_Zed_Ability2::PostPlayMontage()
{
	if (IsLocallyControlled())
	{
		AddTagToAvatar(ZedShadowAbility::TAG_STATUS_SPAWNING_SHADOW);
	}
	else
	{
		// 클라이언트가 앞서 있어 서버의 그림자가 생기기 전에 바꿀 수 있으므로 서버는 바로 재시전을 기다린다.
		// 그 입력은 어빌리티 종료보다 먼저 도착한다(같은 채널, 순서대로).
		WaitForRecast();
	}

	UWorld* World = GetWorld();
	const double ProjectionTime = GetShadowProjectionTime(CursorLocationClamped);
	if ((World != nullptr) && (ProjectionTime > 0.0))
	{
		World->GetTimerManager().SetTimer(ShadowSpawnTimerHandle, this, &ThisClass::SpawnShadow, static_cast<float>(ProjectionTime), false);
	}
	else
	{
		SpawnShadow();
	}
}

void UGA_Zed_Ability2::SpawnShadow()
{
	if (IsValid(SpawnedShadow))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(ShadowSpawnTimerHandle);
	}

	AAssassinsZedShadow* Shadow = AAssassinsZedShadow::SpawnShadow(ShadowClass, GetAvatarActorFromActorInfo(), FTransform(CursorLookAtRotation, CursorLocationClamped), ShadowLifeSpan);
	if (Shadow == nullptr)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	SpawnedShadow = Shadow;
	Shadow->OnEndPlay.AddDynamic(this, &ThisClass::OnShadowEndPlay);

	if (IsLocallyControlled())
	{
		RemoveTagFromAvatar(ZedShadowAbility::TAG_STATUS_SPAWNING_SHADOW);
		WaitForRecast();
	}
}

void UGA_Zed_Ability2::WaitForRecast()
{
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnRecastReleased);
	ReleaseTask->ReadyForActivation();
}

void UGA_Zed_Ability2::OnRecastReleased(float TimeHeld)
{
	UAbilityTask_WaitInputPress* PressTask = UAbilityTask_WaitInputPress::WaitInputPress(this, true);
	PressTask->OnPress.AddDynamic(this, &ThisClass::OnRecastPressed);
	PressTask->ReadyForActivation();
}

void UGA_Zed_Ability2::OnRecastPressed(float TimeWaited)
{
	// 서버에 아직 그림자가 없을 수 있다. 바꾼 클라이언트는 이미 그림자가 있었으므로 지금 만든다.
	if (!IsValid(SpawnedShadow))
	{
		SpawnShadow();
	}

	MoveToShadow();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Zed_Ability2::MoveToShadow()
{
	if (IsValid(SpawnedShadow))
	{
		SpawnedShadow->SwapPlacesWith(GetAssassinsCharacterFromActorInfo());
	}
}

void UGA_Zed_Ability2::OnShadowEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (Actor != SpawnedShadow)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

double UGA_Zed_Ability2::GetShadowProjectionTime(const FVector& ProjectionLocation) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor == nullptr)
	{
		return 0.0;
	}

	const double Speed = FMath::Clamp(ShadowProjectionSpeed, 0.001, FMath::Max(ShadowProjectionSpeed, 0.001));
	return FVector::Dist2D(AvatarActor->GetActorLocation(), ProjectionLocation) / Speed;
}

void UGA_Zed_Ability2::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 그림자가 어빌리티보다 오래 살아서, 그림자의 종료가 다음 활성화에 닿지 않게 푼다.
	if (IsValid(SpawnedShadow))
	{
		SpawnedShadow->OnEndPlay.RemoveDynamic(this, &ThisClass::OnShadowEndPlay);
	}
	SpawnedShadow = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
