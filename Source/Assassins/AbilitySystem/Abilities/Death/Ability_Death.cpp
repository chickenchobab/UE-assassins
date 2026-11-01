#include "AbilitySystem/Abilities/Death/Ability_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "AssassinsGameplayTags.h"
#include "Character/AssassinsCharacter.h"
#include "Character/AssassinsHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Teams/AssassinsTeamBaseActor.h"

UAbility_Death::UAbility_Death(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	bCancelAllAbilities = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// Add the ability trigger tag as default to the CDO.
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = AssassinsGameplayTags::Event_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	if (bCancelAllAbilities)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->CancelAbilities(nullptr, nullptr, this);
		}
	}

	SetCanBeCanceled(false);

	StartDeath();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive())
	{
		PlayDeath();
	}
}

void UAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	check(ActorInfo);

	// Always try to finish the death when the ability ends in case the ability doesn't.
	// This won't do anything if the death hasn't been started.
	FinishDeath();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAbility_Death::StartDeath()
{
	if (UAssassinsHealthComponent* HealthComponent = UAssassinsHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == EAssassinsDeathState::NotDead)
		{
			HealthComponent->StartDeath();
		}
	}
}

void UAbility_Death::FinishDeath()
{
	if (UAssassinsHealthComponent* HealthComponent = UAssassinsHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo()))
	{
		if (HealthComponent->GetDeathState() == EAssassinsDeathState::DeathStarted)
		{
			HealthComponent->FinishDeath();
		}
	}
}

void UAbility_Death::PlayDeath()
{
	bMontageEnded = false;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && IndicatorComponentClass)
	{
		if (USceneComponent* Indicator = Cast<USceneComponent>(AvatarActor->GetComponentByClass(IndicatorComponentClass)))
		{
			Indicator->SetVisibility(false, true);
		}
	}

	UAnimMontage* DeathMontage = nullptr;
	if (const AAssassinsCharacter* AvatarCharacter = GetAssassinsCharacterFromActorInfo())
	{
		const TArray<TObjectPtr<UAnimMontage>>& DeathMontages = AvatarCharacter->GetDeathMontages();
		if (!DeathMontages.IsEmpty())
		{
			DeathMontage = DeathMontages[FMath::RandRange(0, DeathMontages.Num() - 1)];
		}
	}
	else if (const AAssassinsTeamBaseActor* TeamBase = Cast<AAssassinsTeamBaseActor>(AvatarActor))
	{
		DeathMontage = TeamBase->DeadMontage;
	}

	// 재생할 몽타주가 없으면 기다리지 않고 바로 죽음을 마친다.
	if (DeathMontage == nullptr)
	{
		OnMontageEnd();
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage, 1.0f, NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->ReadyForActivation();
}

void UAbility_Death::HandleMontageEnded()
{
	if (bMontageEnded)
	{
		return;
	}

	bMontageEnded = true;
	OnMontageEnd();
}

void UAbility_Death::OnMontageEnd()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
