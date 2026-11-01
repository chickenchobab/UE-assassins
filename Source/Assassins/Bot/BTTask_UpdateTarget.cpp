#include "Bot/BTTask_UpdateTarget.h"

#include "AbilitySystem/AssassinsTargetingLibrary.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTTask_UpdateTarget::UBTTask_UpdateTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Update Target");

	FoundTargetKey.SelectedKeyName = TEXT("FoundTarget");
	FoundTargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, FoundTargetKey), AActor::StaticClass());
	TargetKey.SelectedKeyName = TEXT("TargetEnemy");
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetKey), AActor::StaticClass());
}

void UBTTask_UpdateTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		FoundTargetKey.ResolveSelectedKey(*BlackboardAsset);
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

EBTNodeResult::Type UBTTask_UpdateTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	AActor* FoundTarget = Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(FoundTargetKey.GetSelectedKeyID()));
	Blackboard->SetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID(), FoundTarget);

	AActor* Target = Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID()));
	UAssassinsTargetingLibrary::MarkTargetedByMinion(Target);
	Blackboard->ClearValue(FoundTargetKey.GetSelectedKeyID());

	return IsValid(Target) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

FString UBTTask_UpdateTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s -> %s"), *Super::GetStaticDescription(), *FoundTargetKey.SelectedKeyName.ToString(), *TargetKey.SelectedKeyName.ToString());
}
