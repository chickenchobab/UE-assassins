#include "Character/AssassinsChampion.h"
#include "AbilitySystem/AssassinsAbilitySet.h"
#include "AssassinsLogCategories.h"
#include "Camera/AssassinsCameraComponent.h"
#include "Character/AssassinsChampionSkillState.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/AssassinsPawnData.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "NativeGameplayTags.h"

namespace ChampionAggro
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_AGGRO_CHAMPION_ATTACK, "Status.Aggro.ChampionAttack");
};

AAssassinsChampion::AAssassinsChampion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AutoPossessAI = EAutoPossessAI::Disabled;

	CameraComponent = CreateDefaultSubobject<UAssassinsCameraComponent>(TEXT("CameraComponent"));

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
		InstigatorCharacter->GrantAggroTag(ChampionAggro::TAG_STATUS_AGGRO_CHAMPION_ATTACK);
	}
}
