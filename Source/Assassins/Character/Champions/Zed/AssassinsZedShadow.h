#pragma once

#include "Character/AssassinsPredictedActor.h"
#include "AssassinsZedShadow.generated.h"

class ACharacter;

UCLASS(Abstract)
class ASSASSINS_API AAssassinsZedShadow : public AAssassinsPredictedActor
{
	GENERATED_BODY()

public:

	AAssassinsZedShadow();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static AAssassinsZedShadow* SpawnShadow(TSubclassOf<AAssassinsZedShadow> ShadowClass, AActor* Zed, const FTransform& SpawnTransform, double LifeSpan);

	void SwapPlacesWith(ACharacter* Zed);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Champion|Zed")
	void CallRazorShuriken();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Assassins|Champion|Zed")
	void CallShadowSlash();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Champion|Zed")
	void RegisterWithOwner();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Champion|Zed")
	void UnregisterFromOwner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Assassins|Champion|Zed")
	double ShadowLifeSpan = 0.0;
};
