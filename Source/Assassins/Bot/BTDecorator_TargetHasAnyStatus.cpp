#include "Bot/BTDecorator_TargetHasAnyStatus.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTDecorator_TargetHasAnyStatus::UBTDecorator_TargetHasAnyStatus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Target Has Any Status");

	TargetKey.SelectedKeyName = TEXT("TargetEnemy");
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetKey), AActor::StaticClass());
}

void UBTDecorator_TargetHasAnyStatus::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTDecorator_TargetHasAnyStatus::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s has %s"), *Super::GetStaticDescription(), *TargetKey.SelectedKeyName.ToString(), *StatusTags.ToStringSimple());
}

bool UBTDecorator_TargetHasAnyStatus::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID())) : nullptr;

	// 대상이 사라졌으면 붙들고 있을 대상이 없는 것이므로 참으로 보고 대상을 비우게 한다.
	if (!IsValid(Target))
	{
		return true;
	}

	const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	return AbilitySystem && AbilitySystem->HasAnyMatchingGameplayTags(StatusTags);
}
