#include "Bot/BTDecorator_IsTargetOutOfRange.h"

#include "AbilitySystem/AssassinsTargetingLibrary.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTDecorator_IsTargetOutOfRange::UBTDecorator_IsTargetOutOfRange(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Is Target Out Of Range");

	TargetKey.SelectedKeyName = TEXT("TargetEnemy");
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetKey), AActor::StaticClass());
	RangeKey.SelectedKeyName = TEXT("AggroRange");
	RangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, RangeKey));
}

void UBTDecorator_IsTargetOutOfRange::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
		RangeKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTDecorator_IsTargetOutOfRange::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s beyond %s + %.0f"), *Super::GetStaticDescription(), *TargetKey.SelectedKeyName.ToString(), *RangeKey.SelectedKeyName.ToString(), RangeMargin);
}

bool UBTDecorator_IsTargetOutOfRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIOwner = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIOwner ? AIOwner->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return false;
	}

	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID())) : nullptr;
	if (!IsValid(Target))
	{
		return true;
	}

	const double Range = Blackboard->GetValue<UBlackboardKeyType_Float>(RangeKey.GetSelectedKeyID());
	return UAssassinsTargetingLibrary::GetDistance2DToTargetEdge(Target, Pawn->GetActorLocation()) >= (Range + RangeMargin);
}
