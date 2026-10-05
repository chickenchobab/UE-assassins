// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/Foundation/GA_LocationTargeted_Immediate.h"
#include "GA_Akali_Ability1.generated.h"

class UGameplayEffect;

/**
 * UGA_Akali_Ability1
 *
 * Akali Q, Five Point Strike. Akali throws kunai toward the cursor: the enemies around her, within the angle of the
 * throw, are hit. The owning client strikes when its montage says the throw is over and tells the server, which
 * strikes in the prediction window of the client's strike. Was the GA_Akali_Ability1 blueprint.
 */
UCLASS(Abstract)
class UGA_Akali_Ability1 : public UGA_LocationTargeted_Immediate
{
	GENERATED_BODY()

public:

	UGA_Akali_Ability1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:

	//~UGA_LocationTargeted_Immediate interface
	virtual void PrePlayMontage() override;
	virtual void PostPlayMontage() override;
	//~End of UGA_LocationTargeted_Immediate interface

protected:

	// Half the angle of the throw, around where Akali faces, in degrees.
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
