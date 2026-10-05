// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Champions/Zed/GA_Zed_ImitatedAbility.h"
#include "GA_Zed_Ability1.generated.h"

class AAssassinsProjectile;
class UGameplayEffect;
class UParticleSystem;
class UParticleSystemComponent;

/**
 * UGA_Zed_Ability1
 *
 * Zed Q, Razor Shuriken. Zed throws a shuriken toward the cursor, and each of his shadows throws one along, once they
 * are all there(UGA_Zed_ImitatedAbility). Only the server throws: the shurikens replicate. Was the GA_Zed_Ability1
 * blueprint.
 */
UCLASS(Abstract)
class UGA_Zed_Ability1 : public UGA_Zed_ImitatedAbility
{
	GENERATED_BODY()

public:

	UGA_Zed_Ability1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGameplayAbility interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	//~UGA_LocationTargeted_Immediate interface
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	//~End of UGA_LocationTargeted_Immediate interface

	//~UGA_Zed_ImitatedAbility interface
	virtual FGameplayTag GetComboTag() const override;
	virtual FGameplayTag GetPartnerComboTag() const override;
	virtual void ImitateWith(AAssassinsZedShadow& Shadow) override;
	//~End of UGA_Zed_ImitatedAbility interface

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// Applied instead to the first unit a shuriken goes through.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> FirstHitDamageEffectClass;

	// Follows Zed's hand for a moment as he throws.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UParticleSystem> TrailParticle;

	UPROPERTY(BlueprintReadOnly, Category = "Assassins|Ability")
	TObjectPtr<UParticleSystemComponent> SpawnedParticleSystem;

private:

	void SpawnTrail();

	// Spawns a shuriken from the source, Zed or a shadow, flying the way it faces. Server only.
	void ThrowFrom(AActor* SourceActor);

	UFUNCTION()
	void OnThrowEnded(FGameplayEventData Payload);

	UFUNCTION()
	void OnTrailExpired();
};
