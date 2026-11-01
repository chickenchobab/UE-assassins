#include "Bot/BTDecorator_HasAnyStatus.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTDecorator_HasAnyStatus::UBTDecorator_HasAnyStatus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Has Any Status");
}

FString UBTDecorator_HasAnyStatus::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *StatusTags.ToStringSimple());
}

bool UBTDecorator_HasAnyStatus::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIOwner = OwnerComp.GetAIOwner();
	const UAbilitySystemComponent* AbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIOwner ? AIOwner->GetPawn() : nullptr);
	return AbilitySystem && AbilitySystem->HasAnyMatchingGameplayTags(StatusTags);
}
