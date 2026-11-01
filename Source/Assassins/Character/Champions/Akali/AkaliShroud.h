#pragma once

#include "Character/AssassinsPredictedActor.h"
#include "AkaliShroud.generated.h"

UCLASS(Abstract)
class ASSASSINS_API AAkaliShroud : public AAssassinsPredictedActor
{
	GENERATED_BODY()

public:

	AAkaliShroud();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Assassins|Champion|Akali")
	void IsActorInStealthRadius(AActor* TargetActor, bool& CanStealth) const;

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
