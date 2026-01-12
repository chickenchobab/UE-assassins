// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/StaticMeshActor.h"
#include "AssassinsTeamBaseActor.generated.h"

/**
 * Actor indicating the base(nexus) of a team. Minion waves are spawned here
 * and get the transform of this actor initially.
 */
UCLASS()
class AAssassinsTeamBaseActor : public AStaticMeshActor
{
	GENERATED_BODY()
	
public:

	AAssassinsTeamBaseActor();

	UPROPERTY(EditAnywhere, Category = Teams)
	int32 TeamId;
};
