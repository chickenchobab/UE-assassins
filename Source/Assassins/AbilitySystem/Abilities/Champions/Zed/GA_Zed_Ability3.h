#pragma once

#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_ImitatedAbility.h"
#include "Engine/EngineTypes.h"
#include "GA_Zed_Ability3.generated.h"

class UGameplayEffect;

UCLASS(Abstract)
class UGA_Zed_Ability3 : public UGA_Zed_ImitatedAbility
{
	GENERATED_BODY()

public:

	UGA_Zed_Ability3(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EvaluateCursorTransform(const FGameplayEventData& EventData) override {}
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;

	virtual FGameplayTag GetComboTag() const override;
	virtual FGameplayTag GetPartnerComboTag() const override;
	virtual void ImitateWith(AAssassinsZedShadow& Shadow) override;

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
