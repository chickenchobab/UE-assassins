#pragma once

#include "AbilitySystem/Abilities/Foundation/Ability_UnitTargeted.h"
#include "Ability_Leblanc_Q.generated.h"

class AAssassinsProjectile;
class UGameplayEffect;

UCLASS(Abstract)
class UAbility_Leblanc_Q : public UAbility_UnitTargeted
{
	GENERATED_BODY()

public:

	UAbility_Leblanc_Q(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	virtual FGameplayTagContainer GetPendingCastTags() const override;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<AAssassinsProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double ProjectileSpeed = 0.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	EAbilityCustomReplicatedEvent SpawnProjectileEventType = EAbilityCustomReplicatedEvent::GameCustom1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Ability")
	double HomingAcceleration = 2500.0;

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
