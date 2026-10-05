// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Champions/Leblanc/GA_Leblanc_Attack.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UGA_Leblanc_Attack::UGA_Leblanc_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The socket her attack animation throws from.
	ProjectileSpawnSocket = TEXT("AttackSocket");
}

void UGA_Leblanc_Attack::SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform)
{
	if (!IsValid(SourceActor))
	{
		return;
	}

	const FRotator ActorRotation = SourceActor->GetActorRotation();

	// From her hand when there is a mesh to ask, from where she stands otherwise.
	const ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor);
	const FVector SpawnLocation = SourceCharacter
		? SourceCharacter->GetMesh()->GetSocketLocation(ProjectileSpawnSocket)
		: SourceActor->GetActorLocation();

	SpawnTransform = FTransform(ActorRotation, SpawnLocation, FVector::OneVector);
}
