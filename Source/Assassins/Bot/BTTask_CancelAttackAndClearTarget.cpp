#include "Bot/BTTask_CancelAttackAndClearTarget.h"

#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Character/AssassinsCharacter.h"
#include "NativeGameplayTags.h"

namespace CancelAttackAndClearTarget
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ACTIVATE_ATTACK, "Ability.ActivateAttack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ABILITY_ATTACK, "Ability.Attack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_TARGET_MINION, "Status.Target.Minion");
}

UBTTask_CancelAttackAndClearTarget::UBTTask_CancelAttackAndClearTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Cancel Attack And Clear Target");

	TargetKey.SelectedKeyName = TEXT("TargetEnemy");
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetKey), AActor::StaticClass());
}

void UBTTask_CancelAttackAndClearTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

EBTNodeResult::Type UBTTask_CancelAttackAndClearTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		if (AAssassinsCharacter* TargetCharacter = Cast<AAssassinsCharacter>(Blackboard->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID())))
		{
			TargetCharacter->RemoveGameplayTag(CancelAttackAndClearTarget::TAG_STATUS_TARGET_MINION);
		}
		Blackboard->ClearValue(TargetKey.GetSelectedKeyID());
	}

	const AAIController* AIOwner = OwnerComp.GetAIOwner();
	UAssassinsAbilitySystemComponent* AbilitySystem = Cast<UAssassinsAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIOwner ? AIOwner->GetPawn() : nullptr));
	if (AbilitySystem == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	AbilitySystem->K2_CancelAbilities(CancelAttackAndClearTarget::TAG_ABILITY_ACTIVATE_ATTACK, FGameplayTag());
	AbilitySystem->K2_CancelAbilities(CancelAttackAndClearTarget::TAG_ABILITY_ATTACK, FGameplayTag());
	return EBTNodeResult::Succeeded;
}

FString UBTTask_CancelAttackAndClearTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: clear %s"), *Super::GetStaticDescription(), *TargetKey.SelectedKeyName.ToString());
}
