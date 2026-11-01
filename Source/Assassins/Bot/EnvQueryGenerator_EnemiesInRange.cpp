#include "Bot/EnvQueryGenerator_EnemiesInRange.h"

#include "AbilitySystem/AssassinsTargetingLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Character/AssassinsCharacter.h"
#include "EngineUtils.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "NativeGameplayTags.h"
#include "Teams/AssassinsTeamBaseActor.h"
#include "Teams/AssassinsTeamSubsystem.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator_EnemiesInRange"

namespace EnemiesInRange
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_DEATH, "Status.Death");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_UNTARGETABLE, "Status.Untargetable");
}

UEnvQueryGenerator_EnemiesInRange::UEnvQueryGenerator_EnemiesInRange(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ItemType = UEnvQueryItemType_Actor::StaticClass();
}

void UEnvQueryGenerator_EnemiesInRange::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	AAssassinsCharacter* Querier = Cast<AAssassinsCharacter>(QueryInstance.Owner.Get());
	UWorld* World = Querier ? Querier->GetWorld() : nullptr;
	const UAssassinsTeamSubsystem* TeamSubsystem = World ? World->GetSubsystem<UAssassinsTeamSubsystem>() : nullptr;
	if (TeamSubsystem == nullptr)
	{
		return;
	}

	const UBlackboardComponent* Blackboard = UAIBlueprintHelperLibrary::GetBlackboard(Querier);
	const double Range = Blackboard ? Blackboard->GetValueAsFloat(RangeKeyName) : DefaultRange;
	const FVector QuerierLocation = Querier->GetActorLocation();

	FGameplayTagContainer UnattackableTags;
	UnattackableTags.AddTag(EnemiesInRange::TAG_STATUS_DEATH);
	UnattackableTags.AddTag(EnemiesInRange::TAG_STATUS_UNTARGETABLE);

	auto AddIfEnemyInRange = [&](AActor* Candidate)
	{
		if (!IsValid(Candidate) || (Candidate == Querier))
		{
			return;
		}

		// 능력 시스템이 아직 없는 대상은 태그로 거르지 않는다(이전 BP 생성기와 같다).
		const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate);
		if (AbilitySystem && AbilitySystem->HasAnyMatchingGameplayTags(UnattackableTags))
		{
			return;
		}

		if (UAssassinsTargetingLibrary::GetDistance2DToTargetEdge(Candidate, QuerierLocation) > Range)
		{
			return;
		}

		int32 CandidateTeamId = INDEX_NONE;
		int32 QuerierTeamId = INDEX_NONE;
		if (TeamSubsystem->CompareTeams(Candidate, Querier, CandidateTeamId, QuerierTeamId) == EAssassinsTeamComparison::DifferentTeams)
		{
			QueryInstance.AddItemData<UEnvQueryItemType_Actor>(Candidate);
		}
	};

	for (TActorIterator<AAssassinsCharacter> It(World); It; ++It)
	{
		AddIfEnemyInRange(*It);
	}
	for (TActorIterator<AAssassinsTeamBaseActor> It(World); It; ++It)
	{
		AddIfEnemyInRange(*It);
	}
}

FText UEnvQueryGenerator_EnemiesInRange::GetDescriptionTitle() const
{
	return LOCTEXT("Title", "Enemies In Range");
}

FText UEnvQueryGenerator_EnemiesInRange::GetDescriptionDetails() const
{
	return FText::Format(LOCTEXT("Details", "characters and bases of other teams within blackboard {0} (default {1}) of the querier"),
		FText::FromName(RangeKeyName), FText::AsNumber(DefaultRange));
}

#undef LOCTEXT_NAMESPACE
