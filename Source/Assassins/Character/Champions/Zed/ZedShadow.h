#pragma once

#include "Character/AssassinsPredictedActor.h"
#include "ZedShadow.generated.h"

class ACharacter;

UCLASS(Abstract)
class ASSASSINS_API AZedShadow : public AAssassinsPredictedActor
{
	GENERATED_BODY()

public:

	AZedShadow();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static AZedShadow* SpawnShadow(TSubclassOf<AZedShadow> ShadowClass, AActor* Zed, const FTransform& SpawnTransform, double LifeSpan);

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Assassins|Champion|Zed")
	double DelayForDisappear = 0.0;

protected:

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Assassins|Champion|Zed")
	void OnStartDisappearing();

private:

	void StartDisappearing();

	void FinishDisappearing();

	FTimerHandle LifeSpanTimerHandle;

	FTimerHandle DisappearTimerHandle;
};
