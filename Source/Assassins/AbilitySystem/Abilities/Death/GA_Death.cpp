// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Death/GA_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Character/AssassinsCharacter.h"
#include "Components/SceneComponent.h"
#include "Teams/AssassinsTeamBaseActor.h"

UGA_Death::UGA_Death(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UGA_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Cancels the other abilities and starts the death.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsActive())
	{
		PlayDeath();
	}
}

void UGA_Death::PlayDeath()
{
	bMontageEnded = false;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && IndicatorComponentClass)
	{
		if (USceneComponent* Indicator = Cast<USceneComponent>(AvatarActor->GetComponentByClass(IndicatorComponentClass)))
		{
			Indicator->SetVisibility(false, /*bPropagateToChildren*/ true);
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

	// Nothing to play: the death is over right away.
	if (DeathMontage == nullptr)
	{
		OnMontageEnd();
		return;
	}

	// However the montage stops, the death goes on to its end.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage, /*Rate*/ 1.0f, NAME_None, /*bStopWhenAbilityEnds*/ true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageEnded);
	MontageTask->ReadyForActivation();
}

void UGA_Death::HandleMontageEnded()
{
	if (bMontageEnded)
	{
		return;
	}

	bMontageEnded = true;
	OnMontageEnd();
}

void UGA_Death::OnMontageEnd()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility*/ true, /*bWasCancelled*/ false);
}
