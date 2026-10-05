// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Character/AssassinsPredictedActor.h"
#include "AssassinsZedShadow.generated.h"

class ACharacter;

/**
 * AAssassinsZedShadow
 *
 * Native parent of B_ZedShadow. Declares what Zed's abilities ask the shadows to do, and what Ability2 and Ability4
 * both do with their shadow: spawn it, and swap places with it.
 */
UCLASS(Abstract)
class ASSASSINS_API AAssassinsZedShadow : public AAssassinsPredictedActor
{
	GENERATED_BODY()

public:

	AAssassinsZedShadow();

	//~AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of AActor interface

	// Spawns a shadow of Zed's that lasts LifeSpan, which it reads as it begins play. Null when it could not.
	static AAssassinsZedShadow* SpawnShadow(TSubclassOf<AAssassinsZedShadow> ShadowClass, AActor* Zed, const FTransform& SpawnTransform, double LifeSpan);

	// Zed and the shadow swap places: the shadow goes where Zed stands, and Zed teleports where it stood, as his movement
	// does(UAssassinsCharacterMovementComponent::TeleportCharacter): predicted, it reaches the server with his moves.
	void SwapPlacesWith(ACharacter* Zed);

	// Throws Razor Shuriken(Ability1) from the shadow. Implemented by B_ZedShadow.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Champion|Zed")
	void CallRazorShuriken();

	// Performs Shadow Slash(Ability3) from the shadow. Implemented by B_ZedShadow.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Champion|Zed")
	void CallShadowSlash();

	// Adds the shadow to Zed's skill state, for his Ability1 and Ability3 to make it imitate them. B_ZedShadow calls it
	// as the shadow shows up, except on the copy the owning client hides: that client has a shadow of its own.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Champion|Zed")
	void RegisterWithOwner();

	// Takes the shadow out of Zed's skill state. B_ZedShadow calls it as the shadow starts to go away.
	UFUNCTION(BlueprintCallable, Category = "Assassins|Champion|Zed")
	void UnregisterFromOwner();

	// How long the shadow lasts, set by the ability before the shadow begins play(SpawnActorDeferred). Replicated so
	// that the copies on the clients go away with it. Was the B_ZedShadow variable LifeSpan: that name can't be used,
	// since a variable node of an actor follows the engine redirect Actor.LifeSpan -> InitialLifeSpan on reconstruction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Assassins|Champion|Zed")
	double ShadowLifeSpan = 0.0;
};
