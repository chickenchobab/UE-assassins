// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/AssassinsMinion.h"

#include "Character/AssassinsHealthComponent.h"
#include "NativeGameplayTags.h"

namespace MinionAggro
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_AGGRO_MINION_ATTACK, "Status.Aggro.MinionAttack");
};

AAssassinsMinion::AAssassinsMinion()
{
}

void AAssassinsMinion::BeginPlay()
{
	Super::BeginPlay();

	// The server keeps the aggro: the minions pick their targets there.
	if (HasAuthority())
	{
		if (UAssassinsHealthComponent* MinionHealthComponent = UAssassinsHealthComponent::FindHealthComponent(this))
		{
			MinionHealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::GrantAggroTagToInstigator);
		}
	}
}

void AAssassinsMinion::GrantAggroTagToInstigator(UAssassinsHealthComponent* ChangedHealthComponent, float OldValue, float NewValue, AActor* HealthInstigator)
{
	if (NewValue >= OldValue)
	{
		return;
	}

	if (AAssassinsCharacter* InstigatorCharacter = Cast<AAssassinsCharacter>(HealthInstigator))
	{
		InstigatorCharacter->SetGameplayTag(MinionAggro::TAG_STATUS_AGGRO_MINION_ATTACK);
	}
}
