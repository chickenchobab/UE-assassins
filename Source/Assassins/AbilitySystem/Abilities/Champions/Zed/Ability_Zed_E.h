#pragma once

#include "AbilitySystem/Abilities/Champions/Zed/Ability_Zed_ShadowImitable.h"
#include "Engine/EngineTypes.h"
#include "Ability_Zed_E.generated.h"

class UGameplayEffect;

UCLASS(Abstract)
class UAbility_Zed_E : public UAbility_Zed_ShadowImitable
{
	GENERATED_BODY()

public:

	UAbility_Zed_E(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EvaluateCursorTransform(const FGameplayEventData& EventData) override {}
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;

	virtual FGameplayTag GetComboTag() const override;
	virtual FGameplayTag GetPartnerComboTag() const override;
	virtual void ImitateWith(AZedShadow& Shadow) override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TArray<TEnumAsByte<EObjectTypeQuery>> TypesToSphereTrace;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> SlashTargetEffectClass;

private:

	void SlashAround(AActor* SourceActor);

	void ApplyDamageAndStatus(AActor* HitActor);
};
