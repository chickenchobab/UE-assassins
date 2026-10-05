// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_UnitTargeted.h"
#include "GA_Leblanc_Ability1.generated.h"

class AAssassinsProjectile;
class UGameplayEffect;

/**
 * UGA_Leblanc_Ability1
 *
 * Leblanc Q, Sigil of Malice. Leblanc throws a sigil which homes in on the target. The owning client tells the server
 * when its montage reaches the throw, and only the server throws: the sigil replicates. The mimic of her ultimate
 * (Ability.Ability4) is a blueprint deriving from this one's. Was the GA_Leblanc_Ability1 blueprint.
 */
UCLASS(Abstract)
class UGA_Leblanc_Ability1 : public UGA_UnitTargeted
{
	GENERATED_BODY()

public:

	UGA_Leblanc_Ability1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGA_UnitTargeted interface
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual FGameplayTagContainer GetPendingCastTags() const override;
	//~End of UGA_UnitTargeted interface

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileSpeed = 0.0;

	// Carried by the sigil to the target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// How the owning client asks the server to throw.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	EAbilityCustomReplicatedEvent SpawnProjectileEventType = EAbilityCustomReplicatedEvent::GameCustom1;

	// How hard the sigil turns to stay on its target.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double HomingAcceleration = 2500.0;

	// Where on the mesh the sigil leaves from.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	FName ProjectileSpawnSocket;

private:

	void SpawnProjectile();
	void HandleProjectile(AAssassinsProjectile* SpawnedProjectile);

	UFUNCTION()
	void OnSpawnProjectileNotify(FGameplayEventData Payload);

	UFUNCTION()
	void OnSpawnProjectileRequested();
};
