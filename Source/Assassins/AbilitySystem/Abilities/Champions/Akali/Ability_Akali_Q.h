#pragma once

#include "AbilitySystem/Abilities/Foundation/Ability_LocationTargeted.h"
#include "Ability_Akali_Q.generated.h"

class UGameplayEffect;

UCLASS(Abstract)
class UAbility_Akali_Q : public UAbility_LocationTargeted
{
	GENERATED_BODY()

public:

	UAbility_Akali_Q(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double SectorHalfAngleDegree = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:

	void Strike();
	void ApplyDamageInSector();
	bool IsWithinSectorAngle(const FVector& ImpactPoint) const;

	UFUNCTION()
	void OnStrikeNotify(FGameplayEventData Payload);

	UFUNCTION()
	void OnStrikeRequested();
};
