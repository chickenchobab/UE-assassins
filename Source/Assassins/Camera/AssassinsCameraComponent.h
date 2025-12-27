// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Camera/CameraComponent.h"
#include "AssassinsCameraComponent.generated.h"

class UAssassinsCameraMode;
class UAssassinsCameraModeStack;

DECLARE_DELEGATE_RetVal(TSubclassOf<UAssassinsCameraMode>, FAssassinsCameraModeDelegate);

/**
 * The base camera component class used by this project.
 */
UCLASS()
class UAssassinsCameraComponent : public UCameraComponent
{
	GENERATED_BODY()
	
public:

	UAssassinsCameraComponent(const FObjectInitializer& ObjectInitializer);

	// Returns the camera component if one exists on the specified actor
	UFUNCTION(BlueprintPure, Category = "Assassins|Camera")
	static UAssassinsCameraComponent* FindCameraComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UAssassinsCameraComponent>() : nullptr); }

	// Returns the target actor that the camera is looking at.
	virtual AActor* GetTargetActor() const { return GetOwner(); }

	// Delegate used to query for the best camera mode.
	FAssassinsCameraModeDelegate DetermineCameraModeDelegate;

protected:

	//~UCameraComponent interface
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
	//~End of UCameraComponent interface

	virtual void UpdateCameraMode();

protected:

	void SetCurrentCameraMode(TSubclassOf<UAssassinsCameraMode> CameraModeClass);

protected:

	// Me: Current camera mode used to fill minimal view info.
	UPROPERTY()
	TObjectPtr<UAssassinsCameraMode> CurrentCameraMode;

	// Offset applied to the field of view. The offset is only for one frame, it gets cleared once it is applied.
	float FieldOfViewOffset;
};
