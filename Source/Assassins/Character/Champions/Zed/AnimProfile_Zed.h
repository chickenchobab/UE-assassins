#pragma once

#include "Animation/AssassinsAnimProfile.h"
#include "AnimProfile_Zed.generated.h"

UCLASS()
class UAnimProfile_Zed : public UAssassinsAnimProfile
{
	GENERATED_BODY()

public:

	virtual void ChooseStartAnim(UAssassinsChampionAnimInstance& AnimInstance) override;
	virtual void ChooseStopAnim(UAssassinsChampionAnimInstance& AnimInstance) override;
	virtual void HandleStopAnimStarted() override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Start")
	TObjectPtr<UAnimSequenceBase> DefaultStartSequence;

	UPROPERTY(EditDefaultsOnly, Category = "Stop")
	TArray<TObjectPtr<UAnimSequenceBase>> StopSequences;

private:

	int32 CurrentStopSequenceIndex = 0;
};
