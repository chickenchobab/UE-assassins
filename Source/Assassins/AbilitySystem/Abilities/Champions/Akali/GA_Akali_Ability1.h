#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GA_Akali_Ability1.generated.h"

class UGameplayEffect;

UCLASS(Abstract)
class UGA_Akali_Ability1 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
