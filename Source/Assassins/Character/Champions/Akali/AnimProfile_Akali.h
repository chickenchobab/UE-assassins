#pragma once

#include "Animation/AssassinsAnimProfile.h"
#include "AnimProfile_Akali.generated.h"

class UAnimMontage;

UCLASS()
class UAnimProfile_Akali : public UAssassinsAnimProfile
{
	GENERATED_BODY()

public:

	virtual void UpdateTags(const UAbilitySystemComponent* AbilitySystem) override;
	virtual void ChooseStartAnim(UAssassinsChampionAnimInstance& AnimInstance) override;
	virtual void ChooseStopAnim(UAssassinsChampionAnimInstance& AnimInstance) override;
	virtual void HandleStopAnimStarted() override;
	virtual bool ShouldOverrideIdleSequence(const UAssassinsChampionAnimInstance& AnimInstance) const override;
	virtual bool ShouldOverrideCycleSequence(const UAssassinsChampionAnimInstance& AnimInstance) const override;

protected:

	virtual UAnimSequenceBase* GetIdleSequenceOverride() const override;
	virtual UAnimSequenceBase* GetWalkSequenceOverride() const override;
	virtual UAnimSequenceBase* GetRunSequenceOverride() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Idle")
	TObjectPtr<UAnimSequenceBase> IdleSequenceOverride;

	UPROPERTY(EditDefaultsOnly, Category = "Cycle")
	TObjectPtr<UAnimSequenceBase> WalkSequenceOverride;

	UPROPERTY(EditDefaultsOnly, Category = "Cycle")
	TObjectPtr<UAnimSequenceBase> RunSequenceOverride;

	UPROPERTY(EditDefaultsOnly, Category = "Start")
	TMap<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimSequenceBase>> StartSequenceMap;

	UPROPERTY(EditDefaultsOnly, Category = "Start")
	TMap<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimSequenceBase>> StartSequenceMapWithPassive;

	UPROPERTY(EditDefaultsOnly, Category = "Stop")
	TMap<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimSequenceBase>> StopSequenceMap;

	UPROPERTY(EditDefaultsOnly, Category = "Stop")
	TMap<TObjectPtr<UAnimMontage>, TObjectPtr<UAnimSequenceBase>> StopSequenceMapWithPassive;

	UPROPERTY(EditDefaultsOnly, Category = "Stop")
	TArray<TObjectPtr<UAnimSequenceBase>> StopSequences;

	UPROPERTY(EditDefaultsOnly, Category = "Stop")
	TObjectPtr<UAnimMontage> PassiveMontage;

private:

	int32 CurrentStopSequenceIndex = 0;
	bool bTakeStopInTurn = false;

	bool bHasPassive = false;
	bool bHadPassiveLastUpdate = false;
};
