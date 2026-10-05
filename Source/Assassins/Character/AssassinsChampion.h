#pragma once

#include "Character/AssassinsCharacter.h"
#include "GameplayTagContainer.h"
#include "AssassinsChampion.generated.h"

class UAssassinsCameraComponent;
class UAssassinsChampionSkillState;
class UAssassinsHealthComponent;

UCLASS(Abstract)
class ASSASSINS_API AAssassinsChampion : public AAssassinsCharacter
{
	GENERATED_BODY()

public:

	AAssassinsChampion(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UAssassinsChampionSkillState* GetSkillState();

	template <class T>
	T* GetSkillState() { return Cast<T>(GetSkillState()); }

	template <class T>
	static T* FindSkillState(AActor* Actor)
	{
		AAssassinsChampion* Champion = Cast<AAssassinsChampion>(Actor);
		return Champion ? Champion->GetSkillState<T>() : nullptr;
	}

	UAssassinsCameraComponent* GetAssassinsCameraComponent() const { return CameraComponent; }

protected:

	virtual void BeginPlay() override;

	virtual void OnAbilitySystemInitialized() override;
	virtual void HandleGenericGameplayTagEvent_Implementation(const FGameplayTag Tag, int32 NewCount) override;

private:

	UFUNCTION()
	void GrantAggroTagToInstigator(UAssassinsHealthComponent* ChangedHealthComponent, float OldValue, float NewValue, AActor* HealthInstigator);

	void HandleAggroTagExpired();

	void CreateSkillState();

	FTimerHandle AggroTagTimerHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAssassinsCameraComponent> CameraComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAssassinsChampionSkillState> SkillState;

	bool bSkillStateResolved = false;
};
