// Fill out your copyright notice in the Description page of Project Settings.

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

	// The owning client asks the server to throw the chain when its montage reaches the throw.
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

	// Faces the cursor.
	Super::PrePlayMontage();

	UAssassinsChampionSkillState_Leblanc::SetMimicAbilityOf(*this, LeblancChains::TAG_EVENT_MIMIC_ABILITY3);

	// The server throws when the owning client says its montage reached the throw.
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

	// Leblanc keeps running the way she goes. The channeling tag gives the rotation back when it is over.
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
		this, LeblancChains::TAG_EVENT_SPAWN_PROJECTILE_NOTIFY, /*OptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
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

	// Only the server throws: the chain replicates.
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

	// From Leblanc herself, at the height of the sigil socket, as Zed and Akali throw from where they stand. Where the
	// hand is depends on the server: a listen server animates the mesh, and the throw takes the hand some 150 ahead of
	// her. Thrown from there, the chain passes a target she stands on(after W onto it); from her, it starts on the
	// target and hits it.
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
	// The tether tag is the one watched on the target. The tether is an effect the damage effect applies along with it
	// (its additional effects), which grants the tag: the damage effect grants none itself.
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
		this, LeblancChains::TAG_EVENT_CHAIN_HIT, /*OptionalExternalTarget*/ nullptr, /*OnlyTriggerOnce*/ true, /*OnlyMatchExact*/ true);
	HitTask->EventReceived.AddDynamic(this, &ThisClass::OnChainHit);
	HitTask->ReadyForActivation();
}

void UGA_Leblanc_Ability3::OnChainHit(FGameplayEventData Payload)
{
	bChainHit = true;
	ChainTarget = const_cast<AActor*>(ToRawPtr(Payload.Target));

	// Without the tag of the tether nothing would tell when it wears off: there is no tether to follow.
	if (!TetherEffectTag.IsValid())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
		return;
	}

	// The tether breaks when the target walks out of range.
	UAbilityTask_CustomizeTickTask* TickTask = UAbilityTask_CustomizeTickTask::CustomizeTickTask(this);
	TickTask->OnTickTask.AddDynamic(this, &ThisClass::OnTetherTick);
	TickTask->ReadyForActivation();

	// And the ability ends when the tether wears off. An ability task, so that it ends with the ability and cannot end
	// a later activation.
	UAbilityTask_WaitGameplayTagRemoved* TetherTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(
		this, TetherEffectTag, ChainTarget, /*OnlyTriggerOnce*/ true);
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
	// The target is let go before the tether would have held it: the root comes only when the tether effect runs its
	// course.
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ChainTarget))
	{
		TargetASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TetherEffectTag));
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Leblanc_Ability3::OnTetherRemoved()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}

void UGA_Leblanc_Ability3::OnProjectileDestroyed(AActor* DestroyedActor)
{
	// Only this activation's chain counts.
	if (DestroyedActor != ThrownProjectile)
	{
		return;
	}

	ThrownProjectile = nullptr;

	// The hit event may come in the same frame as the chain goes away.
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
	// The server decides, and its end reaches the client: the chain and the tether live on the server only.
	if (!K2_HasAuthority())
	{
		return;
	}

	// A tether in place ends by itself. A chain on its way may still hit.
	if (bMontageOver && !bChainFlying && !bChainHit)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
	}
}

void UGA_Leblanc_Ability3::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// The chain outlives the ability: its end must not reach the next activation.
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
