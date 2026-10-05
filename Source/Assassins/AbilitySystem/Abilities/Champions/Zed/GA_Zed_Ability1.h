#pragma once

#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_ImitatedAbility.h"
#include "GA_Zed_Ability1.generated.h"

class AAssassinsProjectile;
class UGameplayEffect;
class UParticleSystem;
class UParticleSystemComponent;

UCLASS(Abstract)
class UGA_Zed_Ability1 : public UGA_Zed_ImitatedAbility
{
	GENERATED_BODY()

public:

	UGA_Zed_Ability1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;

	virtual FGameplayTag GetComboTag() const override;
	virtual FGameplayTag GetPartnerComboTag() const override;
	virtual void ImitateWith(AAssassinsZedShadow& Shadow) override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> FirstHitDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UParticleSystem> TrailParticle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UParticleSystemComponent> SpawnedParticleSystem;

private:

	void SpawnTrail();

	void ThrowFrom(AActor* SourceActor);

	UFUNCTION()
	void OnThrowEnded(FGameplayEventData Payload);

	UFUNCTION()
	void OnTrailExpired();
};
