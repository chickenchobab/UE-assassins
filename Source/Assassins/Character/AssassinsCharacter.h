// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularCharacter.h"
#include "AssassinsCharacter.generated.h"

class UAssassinsPawnExtensionComponent;
class UAssassinsHealthComponent;
class UAssassinsAbilitySystemComponent;
struct FGameplayEffectSpec;

UCLASS(Blueprintable)
class AAssassinsCharacter : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AAssassinsCharacter();

	UFUNCTION(BlueprintCallable, Category = "Assassins|Character")
	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponent() const;
	//~IAbilitySystemInterface interface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface interface

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE AAssassinsCharacter* GetAbilityTarget() const { return (AbilityTarget.IsValid() ? AbilityTarget.Get() : nullptr); }
	FORCEINLINE void SetAbilityTarget(AAssassinsCharacter* Target) { AbilityTarget = Target; }
	FORCEINLINE void ClearAbilityTarget() { AbilityTarget = nullptr; }

protected:
	virtual void PossessedBy(AController* NewController) override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	virtual void HandleMoveSpeedChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

private:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAssassinsPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAssassinsHealthComponent> HealthComponent;
	
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	//Me: Target of abilities set when the mouse cursor is on it.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Combat", Meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AAssassinsCharacter> AbilityTarget;
};

