#pragma once

#include "Character/AssassinsChampionSkillState.h"
#include "GameplayTagContainer.h"
#include "AssassinsChampionSkillState_Leblanc.generated.h"

class UGameplayAbility;

UCLASS()
class ASSASSINS_API UAssassinsChampionSkillState_Leblanc : public UAssassinsChampionSkillState
{
	GENERATED_BODY()

public:

	virtual void Initialize() override;

	void SetMimicAbility(const UGameplayAbility& CastAbility, const FGameplayTag& EventTag);

	static void SetMimicAbilityOf(const UGameplayAbility& CastAbility, const FGameplayTag& EventTag);

	const FGameplayTag& GetMimicAbilityEventTag() const { return MimicAbilityEventTag; }

private:

	UPROPERTY()
	FGameplayTag MimicAbilityEventTag;
};
