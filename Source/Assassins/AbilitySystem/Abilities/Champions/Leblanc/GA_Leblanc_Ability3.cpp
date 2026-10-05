#include "AbilitySystem/Abilities/Champions/Leblanc/GA_Leblanc_Ability3.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/AssassinsProjectile.h"
#include "AbilitySystem/Tasks/AbilityTask_CustomizeTickTask.h"
#include "AbilitySystem/Tasks/AbilityTask_WaitReplicatedEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsCharacter.h"
#include "Character/Champions/Leblanc/AssassinsChampionSkillState_Leblanc.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameplayEffectComponents/AdditionalEffectsGameplayEffectComponent.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

namespace LeblancChains
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_MIMIC_ABILITY3, "Event.Champion.Leblanc.Mimic.Ability3");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_SPAWN_PROJECTILE_NOTIFY, "Event.AnimNotify.SpawnProjectile");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_CHAIN_HIT, "Event.Champion.Leblanc.ChainHit");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_TARGET_TETHER, "Status.Target.Leblanc.Tether");

	static constexpr EAbilityCustomReplicatedEvent SpawnProjectileEvent = EAbilityCustomReplicatedEvent::GameCustom1;
};

UGA_Leblanc_Ability3::UGA_Leblanc_Ability3(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProjectileSpawnSocket = TEXT("SigilSocket");
}

void UGA_Leblanc_Ability3::PrePlayMontage()
{
	bMontageOver = false;
	bChainFlying = false;
	bChainHit = false;

	AdjustToMovement();
	AddTagToAvatar(AssassinsGameplayTags::Status_Channeling);

	Super::PrePlayMontage();

	UAssassinsChampionSkillState_Leblanc::SetMimicAbilityOf(*this, LeblancChains::TAG_EVENT_MIMIC_ABILITY3);

	if (K2_HasAuthority() && !IsLocallyControlled())
	{
		UAbilityTask_WaitReplicatedEvent* SpawnRequestTask = UAbilityTask_WaitReplicatedEvent::WaitReplicatedEvent(this, LeblancChains::SpawnProjectileEvent);
		SpawnRequestTask->OnEvent.AddDynamic(this, &ThisClass::OnSpawnProjectileRequested);
		SpawnRequestTask->ReadyForActivation();
	}
}

void UGA_Leblanc_Ability3::AdjustToMovement()
{
	AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo();
	if (AvatarCharacter == nullptr)
	{
		return;
	}

	if (AvatarCharacter->GetVelocity().IsNearlyZero(1.e-4))
	{
		MontageToPlay = IdleMontage;
		return;
	}

	MontageToPlay = RunMontage;
	AvatarCharacter->FreezeRotation();
}

void UGA_Leblanc_Ability3::PostPlayMontage()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	UAbilityTask_WaitGameplayEvent* NotifyTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, LeblancChains::TAG_EVENT_SPAWN_PROJECTILE_NOTIFY, nullptr, true, true);
	NotifyTask->EventReceived.AddDynamic(this, &ThisClass::OnSpawnProjectileNotify);
	NotifyTask->ReadyForActivation();
}

void UGA_Leblanc_Ability3::OnSpawnProjectileNotify(FGameplayEventData Payload)
{
	if (K2_HasAuthority())
	{
		SpawnProjectile();
		return;
	}

	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);
	ServerSetReplicatedEvent(LeblancChains::SpawnProjectileEvent);
}

void UGA_Leblanc_Ability3::OnSpawnProjectileRequested()
{
	SpawnProjectile();
}

void UGA_Leblanc_Ability3::SpawnProjectile()
{
	RemoveTagFromAvatar(AssassinsGameplayTags::Status_Channeling);

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const USkeletalMeshComponent* Mesh = GetOwningComponentFromActorInfo();
	if ((AvatarActor == nullptr) || !IsValid(Mesh) || !ProjectileClass)
	{
		EndIfNothingPending();
		return;
	}

	// 르블랑 자신에게서 표식 소켓 높이로 던진다. 손 위치는 서버에 따라 달라, 리슨 서버는 손이 150쯤 앞에 있고
	// 거기서 던지면 위에 선 대상(W로 올라선 뒤)을 지나친다. 르블랑에게서 던지면 대상 위에서 시작해 맞는다.
	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	const FVector SpawnLocation(AvatarLocation.X, AvatarLocation.Y, Mesh->GetSocketLocation(ProjectileSpawnSocket).Z);
	AAssassinsProjectile* Projectile = SpawnAbilityProjectile(ProjectileClass, FTransform(AvatarActor->GetActorRotation(), SpawnLocation));

	DamageEffectSpecHandle = MakeEffectSpecHandle(DamageEffect);
	HandleChainHit();

	if (Projectile == nullptr)
	{
		EndIfNothingPending();
		return;
	}

	HandleProjectile(Projectile);
}

void UGA_Leblanc_Ability3::HandleProjectile(AAssassinsProjectile* SpawnedProjectile)
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();

	ThrownProjectile = SpawnedProjectile;
	bChainFlying = true;

	SpawnedProjectile->OnDestroyed.AddDynamic(this, &ThisClass::OnProjectileDestroyed);
	SpawnedProjectile->LaunchStraight((AvatarActor ? AvatarActor->GetActorForwardVector() : FVector::ForwardVector) * ProjectileSpeed, static_cast<float>(ProjectileRange), DamageEffectSpecHandle);
}

void UGA_Leblanc_Ability3::HandleChainHit()
{
	// 대상에서 지켜보는 태그는 속박 태그다. 속박은 데미지 효과가 추가 효과로 적용하는 효과가 주고, 데미지 효과는 태그를 주지 않는다.
	TetherEffectTag = FGameplayTag();
	if (DamageEffectSpecHandle.IsValid())
	{
		FGameplayTagContainer GrantedTags;
		DamageEffectSpecHandle.Data->GetAllGrantedTags(GrantedTags);

		const UGameplayEffect* DamageEffectDef = DamageEffectSpecHandle.Data->Def;
		if (const UAdditionalEffectsGameplayEffectComponent* AlongEffects = DamageEffectDef ? DamageEffectDef->FindComponent<UAdditionalEffectsGameplayEffectComponent>() : nullptr)
		{
			for (const FConditionalGameplayEffect& AlongEffect : AlongEffects->OnApplicationGameplayEffects)
			{
				if (const UGameplayEffect* AlongEffectCDO = AlongEffect.EffectClass ? AlongEffect.EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr)
				{
					GrantedTags.AppendTags(AlongEffectCDO->GetGrantedTags());
				}
			}
		}

		const FGameplayTagContainer TetherTags = GrantedTags.Filter(FGameplayTagContainer(LeblancChains::TAG_STATUS_TARGET_TETHER));
		if (!TetherTags.IsEmpty())
		{
			TetherEffectTag = TetherTags.First();
		}
	}

	UAbilityTask_WaitGameplayEvent* HitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, LeblancChains::TAG_EVENT_CHAIN_HIT, nullptr, true, true);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnChainHit);
	HitTask->ReadyForActivation();
}

void UGA_Leblanc_Ability3::OnChainHit(FGameplayEventData Payload)
{
	bChainHit = true;
	ChainTarget = const_cast<AActor*>(ToRawPtr(Payload.Target));

	if (!TetherEffectTag.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAbilityTask_CustomizeTickTask* TickTask = UAbilityTask_CustomizeTickTask::CustomizeTickTask(this);
	TickTask->OnTickTask.AddDynamic(this, &ThisClass::OnTetherTick);
	TickTask->ReadyForActivation();

	// 속박이 풀리면 어빌리티가 끝난다. 어빌리티 태스크로 기다려, 어빌리티와 함께 끝나고 다음 활성화를 끝내지 않게 한다.
	UAbilityTask_WaitGameplayTagRemoved* TetherTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, TetherEffectTag, ChainTarget, true);
	TetherTask->Removed.AddDynamic(this, &ThisClass::OnTetherRemoved);
	TetherTask->ReadyForActivation();
}

void UGA_Leblanc_Ability3::OnTetherTick(float DeltaTime)
{
	if (IsTargetOutOfTetherRange())
	{
		BreakTether();
	}
}

bool UGA_Leblanc_Ability3::IsTargetOutOfTetherRange() const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(ChainTarget) || (AvatarActor == nullptr))
	{
		return true;
	}

	return FVector::Dist2D(AvatarActor->GetActorLocation(), ChainTarget->GetActorLocation()) > TetherRange;
}

void UGA_Leblanc_Ability3::BreakTether()
{
	// 속박이 붙잡기 전에 대상을 놓는다. 속박(뿌리)은 속박 효과가 끝까지 갔을 때만 걸린다.
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ChainTarget))
	{
		TargetASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TetherEffectTag));
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Leblanc_Ability3::OnTetherRemoved()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Leblanc_Ability3::OnProjectileDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor != ThrownProjectile)
	{
		return;
	}

	ThrownProjectile = nullptr;

	// 명중 이벤트가 사슬이 사라지는 프레임에 함께 올 수 있어, 빗나감은 다음 틱에 판단한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::CheckChainMissed);
	}
}

void UGA_Leblanc_Ability3::CheckChainMissed()
{
	bChainFlying = false;

	if (IsActive())
	{
		EndIfNothingPending();
	}
}

void UGA_Leblanc_Ability3::OnMontageComplete()
{
	bMontageOver = true;
	EndIfNothingPending();
}

void UGA_Leblanc_Ability3::OnMontageCancelled()
{
	bMontageOver = true;
	EndIfNothingPending();
}

void UGA_Leblanc_Ability3::EndIfNothingPending()
{
	// 서버가 결정하고 그 종료가 클라이언트에 간다. 사슬과 속박은 서버에만 있다.
	if (!K2_HasAuthority())
	{
		return;
	}

	if (bMontageOver && !bChainFlying && !bChainHit)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Leblanc_Ability3::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 사슬이 어빌리티보다 오래 살아서, 사슬의 종료가 다음 활성화에 닿지 않게 푼다.
	if (IsValid(ThrownProjectile))
	{
		ThrownProjectile->OnDestroyed.RemoveDynamic(this, &ThisClass::OnProjectileDestroyed);
	}
	ThrownProjectile = nullptr;

	ChainTarget = nullptr;
	bMontageOver = false;
	bChainFlying = false;
	bChainHit = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
