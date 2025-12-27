// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"

#include "AssassinsCameraMode.generated.h"

#define UE_API ASSASSINS_API

class UAssassinsCameraComponent;

/**
*  View data produced by the camera mode that is used to blend camera modes.
*/
struct FAssassinsCameraModeView
{
public:

	FAssassinsCameraModeView();

public:

	FVector Location;
	FRotator Rotation;
	FRotator ControlRotation;
	float FieldOfView;
};

/**
 *  Base class for all camera mocdes.
 */
UCLASS(MinimalAPI, Abstract, NotBlueprintable)
class UAssassinsCameraMode : public UObject
{
	GENERATED_BODY()
	
public:

	UE_API UAssassinsCameraMode();

	UAssassinsCameraComponent* GetAssassinsCameraComponent() const;

	UE_API AActor* GetTargetActor() const;

	void EvaluateCameraMode(float DeltaTime, FAssassinsCameraModeView& OutCameraModeView);

protected:
	UE_API virtual void UpdateView(float DeltaTime);

	UE_API virtual FVector GetPivotLocation() const;
	UE_API virtual FRotator GetPivotRotation() const;


protected:

	// A tag that can be queried by gameplay code that cares when a kind of camera mode is active
	// without having to ask about a specific mode (e.g., when aiming downsights to get more accuracy)
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	FGameplayTag CameraTypeTag;

	// View output produced by the camera mode
	FAssassinsCameraModeView View;

	// The horizontal field of view (in degrees)
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "5.0", UIMax = "170", ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView;

	// Minimum view pitch (in degrees)
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMin;
	// Maximum view pitch (in degrees)
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMax;
};

#undef UE_API