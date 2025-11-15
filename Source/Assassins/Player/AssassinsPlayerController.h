// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SubclassOf.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"

#include "AssassinsPlayerController.generated.h"

/** Forward declaration to improve compiling times */
class UInputAction;
class UAssassinsAbilitySystemComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);


UCLASS()
class AAssassinsPlayerController : public ACommonPlayerController
{
	GENERATED_BODY()

public:
	AAssassinsPlayerController();

	UAssassinsAbilitySystemComponent* GetAssassinsAbilitySystemComponent() const;

public:
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* SetDestinationClickAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* SetDestinationTouchAction;

protected:	

	//~APlayerController interface
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface
};


