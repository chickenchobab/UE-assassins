// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Info.h"
#include "AssassinsTeamInfo.generated.h"

class UAssassinsTeamAsset;
class UAssassinsTeamSubsystem;

/**
* Team info which is created based on the team asset and registered to the team subsystem
*/
UCLASS()
class AAssassinsTeamInfo : public AInfo
{
	GENERATED_BODY()

public:

	AAssassinsTeamInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

	UFUNCTION(BlueprintPure, Category = Teams)
	int32 GetTeamId() const { return TeamId; }
	void SetTeamId(int32 NewTeamId) { TeamId = NewTeamId; }

	UFUNCTION(BlueprintPure, Category = Teams)
	UAssassinsTeamAsset* GetTeamAsset() const { return TeamAsset; }
	void SetTeamAsset(UAssassinsTeamAsset* InTeamAsset) { TeamAsset = InTeamAsset; }

	UFUNCTION(BlueprintPure, Category = Teams)
	const FTransform& GetTeamBaseTransform() const { return TeamBaseTransform; }
	void SetTeamBaseTransform(const FTransform& InTransform) { TeamBaseTransform = InTransform; }

protected:
	void TryRegisterWithTeamSubsystem();
	virtual void RegisterWithTeamSubsystem(UAssassinsTeamSubsystem* TeamSubsystem);

private:
	UPROPERTY()
	int32 TeamId;

	UPROPERTY()
	TObjectPtr<UAssassinsTeamAsset> TeamAsset;

	UPROPERTY()
	FTransform TeamBaseTransform;
};
