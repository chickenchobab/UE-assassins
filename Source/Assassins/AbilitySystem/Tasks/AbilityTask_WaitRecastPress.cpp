#include "AbilitySystem/Tasks/AbilityTask_WaitRecastPress.h"

#include "AbilitySystemComponent.h"

UAbilityTask_WaitRecastPress* UAbilityTask_WaitRecastPress::WaitRecastPress(UGameplayAbility* OwningAbility, FGameplayTagContainer BlockingTags, bool bTestAlreadyPressed)
{
	UAbilityTask_WaitRecastPress* Task = NewAbilityTask<UAbilityTask_WaitRecastPress>(OwningAbility);
	Task->BlockingTags = BlockingTags;
	Task->bTestInitialState = bTestAlreadyPressed;
	return Task;
}

void UAbilityTask_WaitRecastPress::Activate()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if ((ASC == nullptr) || (Ability == nullptr))
	{
		EndTask();
		return;
	}

	// 원격 클라이언트의 서버는 클라이언트가 보낸 입력을 받는다. 거절할 입력은 클라이언트가 이미 거절했다. 태스크 시작 전 입력도 센다.
	if (!IsLocallyControlled())
	{
		StartListening();
		if (IsForRemoteClient() && !ASC->CallReplicatedEventDelegateIfSet(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()))
		{
			SetWaitingOnRemotePlayerData();
		}
		return;
	}

	for (const FGameplayTag& Tag : BlockingTags)
	{
		const FDelegateHandle Handle = ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnBlockingTagChanged);
		BlockingTagDelegateHandles.Emplace(Tag, Handle);
	}

	if (IsBlocked())
	{
		return;
	}

	if (bTestInitialState)
	{
		const FGameplayAbilitySpec* Spec = Ability->GetCurrentAbilitySpec();
		if (Spec && Spec->InputPressed)
		{
			OnPressCallback();
			return;
		}
	}

	StartListening();
}

void UAbilityTask_WaitRecastPress::OnBlockingTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 그사이 누른 입력은 거절됐다. 입력이 아직 눌려 있을 수 있어 다음 입력만 센다.
	if (IsBlocked())
	{
		StopListening();
	}
	else
	{
		StartListening();
	}
}

void UAbilityTask_WaitRecastPress::OnPressCallback()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if ((ASC == nullptr) || (Ability == nullptr))
	{
		return;
	}

	StopListening();

	// UAbilityTask_WaitInputPress처럼 클라이언트는 자기 예측 창에서 알리고, 서버는 같은 키의 창에서 처리한다.
	FScopedPredictionWindow ScopedPrediction(ASC, IsPredictingClient());

	if (IsPredictingClient())
	{
		ASC->ServerSetReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey(), ASC->ScopedPredictionKey);
	}
	else
	{
		ASC->ConsumeGenericReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey());
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnPress.Broadcast();
	}
	EndTask();
}

bool UAbilityTask_WaitRecastPress::IsBlocked() const
{
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	return ASC && ASC->HasAnyMatchingGameplayTags(BlockingTags);
}

void UAbilityTask_WaitRecastPress::StartListening()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if ((ASC == nullptr) || PressDelegateHandle.IsValid())
	{
		return;
	}

	PressDelegateHandle = ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()).AddUObject(this, &ThisClass::OnPressCallback);
}

void UAbilityTask_WaitRecastPress::StopListening()
{
	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (ASC && PressDelegateHandle.IsValid())
	{
		ASC->AbilityReplicatedEventDelegate(EAbilityGenericReplicatedEvent::InputPressed, GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(PressDelegateHandle);
	}
	PressDelegateHandle.Reset();
}

void UAbilityTask_WaitRecastPress::OnDestroy(bool bInOwnerFinished)
{
	StopListening();

	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		for (const TPair<FGameplayTag, FDelegateHandle>& TagHandle : BlockingTagDelegateHandles)
		{
			ASC->UnregisterGameplayTagEvent(TagHandle.Value, TagHandle.Key, EGameplayTagEventType::NewOrRemoved);
		}
	}
	BlockingTagDelegateHandles.Empty();

	Super::OnDestroy(bInOwnerFinished);
}
