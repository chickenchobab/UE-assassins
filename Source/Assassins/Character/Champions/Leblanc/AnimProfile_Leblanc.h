#pragma once

#include "Animation/AssassinsAnimProfile.h"
#include "AnimProfile_Leblanc.generated.h"

class UAnimMontage;

UCLASS()
class UAnimProfile_Leblanc : public UAssassinsAnimProfile
{
	GENERATED_BODY()

public:

	virtual void ChooseStartAnim(UAssassinsChampionAnimInstance& AnimInstance) override;
	virtual void ChooseStopAnim(UAssassinsChampionAnimInstance& AnimInstance) override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	TObjectPtr<UAnimMontage> DashMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	TObjectPtr<UAnimSequenceBase> StartSequenceAfterDash;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	TObjectPtr<UAnimSequenceBase> StopSequenceAfterDash;

	UPROPERTY(EditDefaultsOnly, Category = "Stop")
	TObjectPtr<UAnimSequenceBase> DefaultStopSequence;
};
