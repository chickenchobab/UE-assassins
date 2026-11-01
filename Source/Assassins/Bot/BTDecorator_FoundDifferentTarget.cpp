#include "Bot/BTDecorator_FoundDifferentTarget.h"

#include "AbilitySystem/AssassinsTargetingLibrary.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTDecorator_FoundDifferentTarget::UBTDecorator_FoundDifferentTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Found Different Target");

	FoundTargetKey.SelectedKeyName = TEXT("FoundTarget");
	FoundTargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, FoundTargetKey), AActor::StaticClass());
	TargetKey.SelectedKeyName = TEXT("TargetEnemy");
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetKey), AActor::StaticClass());
}

void UBTDecorator_FoundDifferentTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		FoundTargetKey.ResolveSelectedKey(*BlackboardAsset);
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTDecorator_FoundDifferentTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s is not %s"), *Super::GetStaticDescription(), *FoundTargetKey.SelectedKeyName.ToString(), *TargetKey.SelectedKeyName.ToString());
}

bool UBTDecorator_FoundDifferentTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard == nullptr)
	{
		return false;
	}

	const AActor* FoundTarget = Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(FoundTargetKey.GetSelectedKeyID()));
	const AActor* Target = Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID()));
	return (FoundTarget != Target) && UAssassinsTargetingLibrary::IsAttackableTarget(FoundTarget);
}
