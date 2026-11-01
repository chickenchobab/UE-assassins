#pragma once

#include "Character/AssassinsCharacterWithAbilities.h"
#include "AssassinsMinion.generated.h"

class UAssassinsHealthComponent;
class UAssassinsMontageWithTiming;

UCLASS(Abstract)
class AAssassinsMinion : public AAssassinsCharacterWithAbilities
{
	GENERATED_BODY()

public:

	AAssassinsMinion();

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void GrantAggroTagToInstigator(UAssassinsHealthComponent* ChangedHealthComponent, float OldValue, float NewValue, AActor* HealthInstigator);

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Minion")
	double AttackRange = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Minion")
	bool IsSiegeMinion = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Minion")
	TArray<TObjectPtr<UAssassinsMontageWithTiming>> MontageDataArray;
};
