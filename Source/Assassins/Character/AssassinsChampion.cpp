// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/AssassinsChampion.h"
#include "AbilitySystem/AssassinsAbilitySet.h"
#include "AssassinsLogCategories.h"
#include "Camera/AssassinsCameraComponent.h"
#include "Character/AssassinsChampionSkillState.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/AssassinsPawnData.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "NativeGameplayTags.h"
#include "TimerManager.h"

namespace ChampionAggro
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_AGGRO_CHAMPION_ATTACK, "Status.Aggro.ChampionAttack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_AGGRO_MINION_ATTACK, "Status.Aggro.MinionAttack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_TARGET_MINION, "Status.Target.Minion");

	// How long an aggro tag lasts after it was granted last.
	static constexpr float AggroTagDuration = 3.0f;
};

AAssassinsChampion::AAssassinsChampion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Champions are possessed by players, or by the bots the game mode spawns.
	AutoPossessAI = EAutoPossessAI::Disabled;

	CameraComponent = CreateDefaultSubobject<UAssassinsCameraComponent>(TEXT("CameraComponent"));

	// What the players see of the champions follows the server closely.
	SetNetUpdateFrequency(200.0f);
}

void AAssassinsChampion::BeginPlay()
{
	Super::BeginPlay();

	if (UAssassinsHealthComponent* ChampionHealthComponent = UAssassinsHealthComponent::FindHealthComponent(this))
	{
		ChampionHealthComponent->OnHealthChanged.AddDynamic(this, &ThisClass::GrantAggroTagToInstigator);
	}
}

UAssassinsChampionSkillState* AAssassinsChampion::GetSkillState()
{
	if (!bSkillStateResolved)
	{
		CreateSkillState();
	}

	return SkillState;
}

void AAssassinsChampion::OnAbilitySystemInitialized()
{
	Super::OnAbilitySystemInitialized();

	if (!bSkillStateResolved)
	{
		CreateSkillState();
	}
}

void AAssassinsChampion::CreateSkillState()
{
	const UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(this);
	const UAssassinsPawnData* PawnData = PawnExtComp ? PawnExtComp->GetPawnData<UAssassinsPawnData>() : nullptr;
	if (PawnData == nullptr)
	{
		return;
	}

	bSkillStateResolved = true;

	TSubclassOf<UAssassinsChampionSkillState> SkillStateClass;
	for (const UAssassinsAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if ((AbilitySet == nullptr) || !AbilitySet->GetSkillStateClass())
		{
			continue;
		}

		if (SkillStateClass)
		{
			UE_LOG(LogAssassins, Error, TEXT("Pawn data [%s] has more than one ability set naming a skill state: [%s] is left out."), *GetNameSafe(PawnData), *GetNameSafe(AbilitySet));
			continue;
		}

		SkillStateClass = AbilitySet->GetSkillStateClass();
	}

	if (SkillStateClass)
	{
		SkillState = NewObject<UAssassinsChampionSkillState>(this, SkillStateClass);
		SkillState->Initialize();
	}
}

void AAssassinsChampion::GrantAggroTagToInstigator(UAssassinsHealthComponent* ChangedHealthComponent, float OldValue, float NewValue, AActor* HealthInstigator)
{
	if (NewValue >= OldValue)
	{
		return;
	}

	if (AAssassinsCharacter* InstigatorCharacter = Cast<AAssassinsCharacter>(HealthInstigator))
	{
		InstigatorCharacter->SetGameplayTag(ChampionAggro::TAG_STATUS_AGGRO_CHAMPION_ATTACK);
	}
}

void AAssassinsChampion::HandleGenericGameplayTagEvent_Implementation(const FGameplayTag Tag, int32 NewCount)
{
	if (!HasAuthority())
	{
		return;
	}

	// No minion targets the champion anymore: the aggro tags go away at once.
	if ((Tag == ChampionAggro::TAG_STATUS_TARGET_MINION) && (NewCount == 0))
	{
		GetWorldTimerManager().ClearTimer(AggroTagTimerHandle);
		ClearGameplayTag(ChampionAggro::TAG_STATUS_AGGRO_CHAMPION_ATTACK);
		ClearGameplayTag(ChampionAggro::TAG_STATUS_AGGRO_MINION_ATTACK);
		return;
	}

	// An aggro tag was granted: it lasts AggroTagDuration from the last grant.
	const bool bIsAggroTag = (Tag == ChampionAggro::TAG_STATUS_AGGRO_CHAMPION_ATTACK) || (Tag == ChampionAggro::TAG_STATUS_AGGRO_MINION_ATTACK);
	if (bIsAggroTag && (NewCount > 0))
	{
		GetWorldTimerManager().ClearTimer(AggroTagTimerHandle);
		GetWorldTimerManager().SetTimer(AggroTagTimerHandle, this, &ThisClass::HandleAggroTagExpired, ChampionAggro::AggroTagDuration, false);
	}
}

void AAssassinsChampion::HandleAggroTagExpired()
{
	// Minions still targeting the champion keep the aggro.
	if (!HasGameplayTag(ChampionAggro::TAG_STATUS_TARGET_MINION))
	{
		ClearGameplayTag(ChampionAggro::TAG_STATUS_AGGRO_CHAMPION_ATTACK);
		ClearGameplayTag(ChampionAggro::TAG_STATUS_AGGRO_MINION_ATTACK);
	}
}
