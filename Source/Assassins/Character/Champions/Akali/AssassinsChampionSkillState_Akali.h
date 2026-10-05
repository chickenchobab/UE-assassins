#pragma once

#include "Character/AssassinsChampionSkillState.h"
#include "AssassinsChampionSkillState_Akali.generated.h"

class AAssassinsProjectile;
class UCapsuleComponent;

UCLASS()
class ASSASSINS_API UAssassinsChampionSkillState_Akali : public UAssassinsChampionSkillState
{
	GENERATED_BODY()

public:

	virtual void Initialize() override;

	UCapsuleComponent* AcquireUltimateCapsule(const UObject* User);
	void ReleaseUltimateCapsule(const UObject* User);

	const UCapsuleComponent* GetUltimateCapsule() const { return UltimateCapsule; }

	UPROPERTY()
	TObjectPtr<AAssassinsProjectile> SpawnedProjectile;

private:

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> UltimateCapsule;

	TSet<TWeakObjectPtr<const UObject>> UltimateCapsuleUsers;
};
