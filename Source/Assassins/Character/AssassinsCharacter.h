// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "AssassinsCharacter.generated.h"

class UAssassinsPawnExtensionComponent;
class UAssassinsHealthComponent;

UCLASS(Blueprintable)
class AAssassinsCharacter : public AModularCharacter
{
	GENERATED_BODY()

public:
	AAssassinsCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

private:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAssassinsPawnExtensionComponent> PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assassins|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr< UAssassinsHealthComponent> HealthComponent;
	
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;
};

