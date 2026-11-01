#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "GameplayTagContainer.h"
#include "AbilityTask_WaitRecastPress.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWaitRecastPressDelegate);

UCLASS()
class ASSASSINS_API UAbilityTask_WaitRecastPress : public UAbilityTask
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FWaitRecastPressDelegate OnPress;

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_WaitRecastPress* WaitRecastPress(UGameplayAbility* OwningAbility, FGameplayTagContainer BlockingTags, bool bTestAlreadyPressed = false);

	virtual void Activate() override;

protected:

	virtual void OnDestroy(bool bInOwnerFinished) override;

private:

	void OnPressCallback();
	void OnBlockingTagChanged(const FGameplayTag Tag, int32 NewCount);

	bool IsBlocked() const;
	void StartListening();
	void StopListening();

	FGameplayTagContainer BlockingTags;
	bool bTestInitialState = false;

	FDelegateHandle PressDelegateHandle;
	TArray<TPair<FGameplayTag, FDelegateHandle>> BlockingTagDelegateHandles;
};
