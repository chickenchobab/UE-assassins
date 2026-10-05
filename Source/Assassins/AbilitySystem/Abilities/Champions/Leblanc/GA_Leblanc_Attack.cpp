#include "AbilitySystem/Abilities/Champions/Leblanc/GA_Leblanc_Attack.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UGA_Leblanc_Attack::UGA_Leblanc_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProjectileSpawnSocket = TEXT("AttackSocket");
}

void UGA_Leblanc_Attack::SetProjectileSpawnTransform_Implementation(AActor* SourceActor, FTransform& SpawnTransform)
{
	if (!IsValid(SourceActor))
	{
		return;
	}

	const FRotator ActorRotation = SourceActor->GetActorRotation();

	const ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor);
	const FVector SpawnLocation = SourceCharacter
		? SourceCharacter->GetMesh()->GetSocketLocation(ProjectileSpawnSocket)
		: SourceActor->GetActorLocation();

	SpawnTransform = FTransform(ActorRotation, SpawnLocation, FVector::OneVector);
}
