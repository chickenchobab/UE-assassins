#pragma once

#include "Animation/AnimNodeReference.h"
#include "UObject/Object.h"
#include "AssassinsAnimProfile.generated.h"

class UAbilitySystemComponent;
class UAnimSequence;
class UAnimSequenceBase;
class UAssassinsChampionAnimInstance;

UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class UAssassinsAnimProfile : public UObject
{
	GENERATED_BODY()

public:

	virtual void UpdateTags(const UAbilitySystemComponent* AbilitySystem) {}

	virtual void ChooseStartAnim(UAssassinsChampionAnimInstance& AnimInstance) PURE_VIRTUAL(UAssassinsAnimProfile::ChooseStartAnim, );
	virtual void ChooseStopAnim(UAssassinsChampionAnimInstance& AnimInstance) PURE_VIRTUAL(UAssassinsAnimProfile::ChooseStopAnim, );
	virtual void HandleStopAnimStarted() {}

	virtual bool ShouldOverrideIdleSequence(const UAssassinsChampionAnimInstance& AnimInstance) const { return false; }
	virtual bool ShouldOverrideCycleSequence(const UAssassinsChampionAnimInstance& AnimInstance) const { return false; }

	bool HasIdleBreaks() const { return IdleBreaks.Num() > 0; }

	void SetupIdleAnim(const FAnimNodeReference& Node) const;
	void SetupIdleOverrideAnim(const FAnimNodeReference& Node) const;
	void SetupWalkAnim(const FAnimNodeReference& Node) const;
	void SetupWalkOverrideAnim(const FAnimNodeReference& Node) const;
	void SetupRunAnim(const FAnimNodeReference& Node) const;
	void SetupRunOverrideAnim(const FAnimNodeReference& Node) const;
	void SetupStartAnim(const FAnimNodeReference& Node) const;
	void SetupStopAnim(const FAnimNodeReference& Node) const;

	void SetupIdleBreakAnim(const FAnimNodeReference& Node);

protected:

	virtual UAnimSequenceBase* GetIdleSequenceOverride() const { return nullptr; }
	virtual UAnimSequenceBase* GetWalkSequenceOverride() const { return nullptr; }
	virtual UAnimSequenceBase* GetRunSequenceOverride() const { return nullptr; }

	static UAnimSequenceBase* TakeInTurn(const TArray<TObjectPtr<UAnimSequenceBase>>& Sequences, int32& Index);

	UPROPERTY(EditDefaultsOnly, Category = "Idle")
	TObjectPtr<UAnimSequenceBase> IdleSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Idle")
	TArray<TObjectPtr<UAnimSequence>> IdleBreaks;

	UPROPERTY(EditDefaultsOnly, Category = "Cycle")
	TObjectPtr<UAnimSequenceBase> WalkSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Cycle")
	TObjectPtr<UAnimSequenceBase> RunSequence;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> StartSequence;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> StopSequence;

private:

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Idle")
	int32 CurrentIdleBreakIndex = 0;
};
