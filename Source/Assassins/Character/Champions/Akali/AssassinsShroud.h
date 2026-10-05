#pragma once

#include "Character/AssassinsPredictedActor.h"
#include "AssassinsShroud.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAssassinsShroudOwnerDelegate);

UCLASS(Abstract)
class ASSASSINS_API AAssassinsShroud : public AAssassinsPredictedActor
{
	GENERATED_BODY()

public:

	AAssassinsShroud();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Assassins|Champion|Akali")
	void IsActorInStealthRadius(AActor* TargetActor, bool& CanStealth) const;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Assassins|Champion|Akali")
	FAssassinsShroudOwnerDelegate OnOwnerInStealthRadius;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Assassins|Champion|Akali")
	FAssassinsShroudOwnerDelegate OnOwnerOutOfStealthRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Champion|Akali")
	double InnerRadius = 220.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Champion|Akali")
	double OuterRadius = 470.0;

	UPROPERTY(BlueprintReadWrite, Category = "Assassins|Champion|Akali")
	FVector DashMarkLocation;

private:

	void HandleOwnerProjectile();

	FVector CalculateDashMarkLocation(const AActor* Projectile) const;

	UFUNCTION(Client, Reliable)
	void ClientReceiveDashMarkLocation(FVector MarkLocation);

	UFUNCTION()
	void OnHandledProjectileDestroyed(AActor* DestroyedActor);

	bool bCanHandleProjectile = true;
};
