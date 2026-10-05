// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/AssassinsPredictedActor.h"
#include "AssassinsShroud.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAssassinsShroudOwnerDelegate);

/**
 * AAssassinsShroud
 *
 * Native parent of B_Shroud, the smoke of Akali's Ability2.
 * Declares what Akali's abilities read from the shroud. The names match the blueprint members they replaced.
 * On the server it also marks where Ability3 dashes when its shuriken flies over the ring, which B_Shroud did.
 */
UCLASS(Abstract)
class ASSASSINS_API AAssassinsShroud : public AAssassinsPredictedActor
{
	GENERATED_BODY()

public:

	AAssassinsShroud();

	//~AActor interface
	virtual void Tick(float DeltaSeconds) override;
	//~End of AActor interface

	// Whether the actor stands in the ring between InnerRadius and OuterRadius, on the XY plane.
	UFUNCTION(BlueprintPure, Category = "Assassins|Champion|Akali")
	void IsActorInStealthRadius(AActor* TargetActor, bool& CanStealth) const;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Assassins|Champion|Akali")
	FAssassinsShroudOwnerDelegate OnOwnerInStealthRadius;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Assassins|Champion|Akali")
	FAssassinsShroudOwnerDelegate OnOwnerOutOfStealthRadius;

	// Doubles, to match the blueprint variables they replace.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Champion|Akali")
	double InnerRadius = 220.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Champion|Akali")
	double OuterRadius = 470.0;

	// Where Ability3 dashes when it has no target.
	UPROPERTY(BlueprintReadWrite, Category = "Assassins|Champion|Akali")
	FVector DashMarkLocation;

private:

	// The shuriken of Ability3 in the ring marks where Akali dashes: halfway between the radii, on the way from the
	// center to the shuriken. Once per shuriken, which is then handed the shroud as what it hit. Server only.
	void HandleOwnerProjectile();

	FVector CalculateDashMarkLocation(const AActor* Projectile) const;

	UFUNCTION(Client, Reliable)
	void ClientReceiveDashMarkLocation(FVector MarkLocation);

	UFUNCTION()
	void OnHandledProjectileDestroyed(AActor* DestroyedActor);

	bool bCanHandleProjectile = true;
};
