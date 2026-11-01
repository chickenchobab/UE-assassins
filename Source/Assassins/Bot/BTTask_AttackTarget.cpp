#include "Bot/BTTask_AttackTarget.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Character/AssassinsCharacter.h"
#include "NativeGameplayTags.h"

namespace AttackTarget
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EVENT_ATTACK, "Event.Attack");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ABILITY_ATTACKING, "Status.Ability.Attacking");

	struct FAttackTargetMemory
	{
		uint64 EventFrame = 0;
	};
}

UBTTask_AttackTarget::UBTTask_AttackTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Attack Target");
	bNotifyTick = true;

	TargetKey.SelectedKeyName = TEXT("TargetEnemy");
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetKey), AActor::StaticClass());
}

void UBTTask_AttackTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIOwner = OwnerComp.GetAIOwner();
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	FGameplayEventData Payload;
	Payload.Target = Blackboard ? Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(TargetKey.GetSelectedKeyID())) : nullptr;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AIOwner ? AIOwner->GetPawn() : nullptr, AttackTarget::TAG_EVENT_ATTACK, Payload);

	// 이벤트로 켜진 공격 어빌리티가 공격 중 태그를 붙였는지는 다음 프레임에 본다(이전 BP의 DelayUntilNextFrame).
	CastInstanceNodeMemory<AttackTarget::FAttackTargetMemory>(NodeMemory)->EventFrame = GFrameCounter;
	return EBTNodeResult::InProgress;
}

void UBTTask_AttackTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (CastInstanceNodeMemory<AttackTarget::FAttackTargetMemory>(NodeMemory)->EventFrame == GFrameCounter)
	{
		return;
	}

	const AAIController* AIOwner = OwnerComp.GetAIOwner();
	const AAssassinsCharacter* Character = Cast<AAssassinsCharacter>(AIOwner ? AIOwner->GetPawn() : nullptr);
	const bool bAttacking = Character && Character->HasMatchingGameplayTag(AttackTarget::TAG_STATUS_ABILITY_ATTACKING);
	FinishLatentTask(OwnerComp, bAttacking ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}

uint16 UBTTask_AttackTarget::GetInstanceMemorySize() const
{
	return sizeof(AttackTarget::FAttackTargetMemory);
}

void UBTTask_AttackTarget::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<AttackTarget::FAttackTargetMemory>(NodeMemory, InitType);
}

void UBTTask_AttackTarget::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<AttackTarget::FAttackTargetMemory>(NodeMemory, CleanupType);
}

FString UBTTask_AttackTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *TargetKey.SelectedKeyName.ToString());
}
