// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssassinsCharacter.h"

#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "UObject/ConstructorHelpers.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Character/AssassinsHealthComponent.h"
#include "Camera/AssassinsCameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "AssassinsLogCategories.h"

AAssassinsCharacter::AAssassinsCharacter()
{
	PawnExtComponent = CreateDefaultSubobject<UAssassinsPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
	 
	HealthComponent = CreateDefaultSubobject<UAssassinsHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::HandleDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::HandleDeathFinished);

	// Configure player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1 /*targeting trace*/, ECR_Block);

    // Me: Use the capsule component as the sole collision handler; disable mesh collision.
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 1500.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	GetCharacterMovement()->MaxAcceleration = 100000.f; // Me: For instant attainment of the specified speed(combat set)
	GetCharacterMovement()->GetNavMovementProperties()->bUseFixedBrakingDistanceForPaths = true;
	GetCharacterMovement()->GetNavMovementProperties()->FixedPathBrakingDistance = 0; // Me: For instant stop after navigation

	// Create a camera...
	CameraComponent = CreateDefaultSubobject<UAssassinsCameraComponent>(TEXT("CameraComponent"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

UAssassinsAbilitySystemComponent* AAssassinsCharacter::GetAssassinsAbilitySystemComponent() const
{
	return Cast<UAssassinsAbilitySystemComponent>(GetAbilitySystemComponent());
}

UAbilitySystemComponent* AAssassinsCharacter::GetAbilitySystemComponent() const
{
	if (PawnExtComponent == nullptr)
	{
		return nullptr;
	}

	return PawnExtComponent->GetAssassinsAbilitySystemComponent();
}

void AAssassinsCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool AAssassinsCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{ 
		return ASC->HasMatchingGameplayTag(TagToCheck);
	}
	
	return false;
}

bool AAssassinsCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		return ASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool AAssassinsCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		return ASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

void AAssassinsCharacter::SetGameplayTag(FGameplayTag Tag)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->SetLooseGameplayTagCount(Tag, 1);
	}
}

void AAssassinsCharacter::UnsetGameplayTag(FGameplayTag Tag)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->SetLooseGameplayTagCount(Tag, 0);
	}
}

void AAssassinsCharacter::AddGameplayTag(FGameplayTag Tag)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(Tag);
	}
}

void AAssassinsCharacter::RemoveGameplayTag(FGameplayTag Tag)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(Tag))
		{
			ASC->RemoveLooseGameplayTag(Tag);
		}
	}
}

bool AAssassinsCharacter::HasGameplayTag(FGameplayTag Tag)
{
	return HasMatchingGameplayTag(Tag);
}

void AAssassinsCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	UE_LOG(LogAssassinsTeams, Error, TEXT("You can't set the team ID on a character (%s) except on the authority"), *GetPathNameSafe(this));
}

FGenericTeamId AAssassinsCharacter::GetGenericTeamId() const
{
	return MyTeamID;
}

void AAssassinsCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PawnExtComponent->HandleControllerChanged();
}

void AAssassinsCharacter::UnPossessed()
{
	Super::UnPossessed();

	PawnExtComponent->HandleControllerChanged();
}

void AAssassinsCharacter::NotifyControllerChanged()
{
	const FGenericTeamId OldTeamId = GetGenericTeamId();

	Super::NotifyControllerChanged();

	// Update our team ID based on the controller (player state actually)
	if (HasAuthority() && (GetController() != nullptr))
	{
		if (IAssassinsTeamAgentInterface* ControllerWithTeam = Cast<IAssassinsTeamAgentInterface>(GetController()))
		{
			MyTeamID = ControllerWithTeam->GetGenericTeamId();
		}
	}
}

void AAssassinsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}

void AAssassinsCharacter::SetTeamId(const FGenericTeamId& NewTeamID)
{
	MyTeamID = NewTeamID;
}

void AAssassinsCharacter::OnAbilitySystemInitialized()
{
	UAssassinsAbilitySystemComponent* AssassinsASC = GetAssassinsAbilitySystemComponent();
	check(AssassinsASC);

	HealthComponent->InitializeWithAbilitySystem(AssassinsASC);

	const UAssassinsCombatSet* CombatSet = AssassinsASC->GetSet<UAssassinsCombatSet>();
	if (CombatSet)
	{
		GetCharacterMovement()->MaxWalkSpeed = CombatSet->GetMoveSpeed();
		// Me: GetSet returns const pointer but member delegates are set mutable
		CombatSet->OnMoveSpeedChanged.AddUObject(this, &AAssassinsCharacter::HandleMoveSpeedChanged);
	}

	InitializeGameplayTags();
}

void AAssassinsCharacter::OnAbilitySystemUninitialized()
{
	HealthComponent->UninitializeFromAbilitySystem();
}

void AAssassinsCharacter::InitializeGameplayTags()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTags(CharacterOwnedTags);

		ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &ThisClass::HandleGenericGameplayTagEvent);
	}
}

void AAssassinsCharacter::HandleDeathStarted()
{
	if (GetController())
	{
		GetController()->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	check(Capsule);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	check(MovementComponent);
	MovementComponent->StopMovementImmediately();
	MovementComponent->DisableMovement();
}

void AAssassinsCharacter::HandleDeathFinished()
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
}

void AAssassinsCharacter::DestroyDueToDeath()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	SetActorHiddenInGame(true);
}

void AAssassinsCharacter::HandleMoveSpeedChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	GetCharacterMovement()->MaxWalkSpeed = NewValue;
}
