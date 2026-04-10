// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssassinsCharacter.h"

#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "UObject/ConstructorHelpers.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
#include "Camera/AssassinsCameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "AssassinsLogCategories.h"
#include "NativeGameplayTags.h"
#include "Player/AssassinsPlayerController.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_CHANNELING, "Status.Channeling");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_UNTARGETABLE, "Status.Untargetable");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_INVISIBLE, "Status.Untargetable.Invisible");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_DASHING, "Status.Dashing");

AAssassinsCharacter::AAssassinsCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAssassinsCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PawnExtComponent = CreateDefaultSubobject<UAssassinsPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));
	 
	HealthComponent = CreateDefaultSubobject<UAssassinsHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::HandleDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::HandleDeathFinished);

	// Configure player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("AssassinsPawn"));

    // Me: Use the capsule component as the sole collision handler; disable mesh collision.
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(-1.f, -1.f, -1.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 50.0f;

	GetCharacterMovement()->MaxAcceleration = 100000.f; // Me: For instant attainment of the specified speed(combat set)
	GetCharacterMovement()->GetNavMovementProperties()->bUseFixedBrakingDistanceForPaths = true;
	GetCharacterMovement()->GetNavMovementProperties()->FixedPathBrakingDistance = 0; // Me: For instant stop after navigation

	GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = false;

	// Create a camera...
	CameraComponent = CreateDefaultSubobject<UAssassinsCameraComponent>(TEXT("CameraComponent"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SetNetUpdateFrequency(200.0f);
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

void AAssassinsCharacter::ClearGameplayTag(FGameplayTag Tag)
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

void AAssassinsCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MyTeamID);
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
	Super::NotifyControllerChanged();

	AController* C = GetController();
	
	// Update our team ID based on the controller (player state actually)
	// Me: Team ID must be set in authority and replicated since simulated proxies have no controller.
	if (HasAuthority())
	{
		if (IAssassinsTeamAgentInterface* ControllerWithTeam = Cast<IAssassinsTeamAgentInterface>(C))
		{
			FGenericTeamId NewTeamID = ControllerWithTeam->GetGenericTeamId();
			SetTeamId(NewTeamID);
		}
	}
}

void AAssassinsCharacter::NotifyRestarted()
{
	Super::NotifyRestarted();

	if (AAssassinsPlayerController* PC = Cast<AAssassinsPlayerController>(GetController()))
	{
		PC->SetPlayerRestarted(true);
		PC->OnPlayerRestarted.Broadcast(this);
	}
}

void AAssassinsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}

void AAssassinsCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void AAssassinsCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PawnExtComponent->HandlePlayerStateReplicated();
}

void AAssassinsCharacter::SetTeamId(const FGenericTeamId& NewTeamID)
{
	const FGenericTeamId OldTeamID = MyTeamID;
	MyTeamID = NewTeamID;

	OnRep_MyTeamID(OldTeamID);
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

		ASC->RegisterGameplayTagEvent(TAG_STATUS_CHANNELING).AddUObject(this, &ThisClass::OnChannelingTagChanged);
		ASC->RegisterGameplayTagEvent(TAG_STATUS_UNTARGETABLE).AddUObject(this, &ThisClass::OnUntargetableTagChanged);
		ASC->RegisterGameplayTagEvent(TAG_STATUS_INVISIBLE).AddUObject(this, &ThisClass::OnInvisibleTagChanged);
		ASC->RegisterGameplayTagEvent(TAG_STATUS_DASHING).AddUObject(this, &ThisClass::OnDashingTagChanged);
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
	Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel3/*ground*/, ECR_Block);

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

void AAssassinsCharacter::ResolvePenetrationAfterDash()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	check(Capsule);

	Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);

	// Find one world static actor overlapping(the map is not designed to have multiple ones)
	GetWorld()->OverlapMultiByChannel(Overlaps, Capsule->GetComponentLocation(), Capsule->GetComponentQuat(), ECC_Pawn, Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	if (!Overlaps.IsEmpty())
	{
		FHitResult Hit;
		GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(), GetActorLocation() - Capsule->GetScaledCapsuleRadius() * GetActorForwardVector() /*The character should be popped forward here*/, Capsule->GetComponentQuat(), ECC_Pawn, Capsule->GetCollisionShape(), QueryParams, ResponseParams);

		if (Hit.bStartPenetrating && Hit.GetActor() == Overlaps.Top().GetActor())
		{
			const FVector RequestedAdjustment = GetMovementComponent()->GetPenetrationAdjustment(Hit);
			bool bAdjusted = GetMovementComponent()->ResolvePenetration(RequestedAdjustment, Hit, Capsule->GetComponentQuat());
			// Teleport if the penetration has not been resolved.
			if (!bAdjusted)
			{
				FVector TeleportLocation = GetActorLocation();
				if (GetWorld()->FindTeleportSpot(this, TeleportLocation, GetActorRotation()))
				{
					SetActorLocation(TeleportLocation);
				}
			}
		}
	}

	Overlaps.Empty();
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Overlap);
	GetWorld()->OverlapMultiByChannel(Overlaps, Capsule->GetComponentLocation(), Capsule->GetComponentQuat(), ECC_Pawn, Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	
	if (Overlaps.IsEmpty())
	{
		Capsule->OnComponentEndOverlap.RemoveDynamic(this, &ThisClass::OnEndPawnOverlapAfterDash);

		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bUseRVOAvoidance = true;
		}

		return;
	}

	Capsule->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnEndPawnOverlapAfterDash);
	for (const FOverlapResult& Result : Overlaps)
	{
		ActorsOverlappedAfterDash.Add(Result.GetActor());
		UE_LOG(LogTemp, Display, TEXT("Found overlapping pawn : [%s]"), *Result.GetActor()->GetName());
	}
}

void AAssassinsCharacter::OnEndPawnOverlapAfterDash(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!ActorsOverlappedAfterDash.Contains(OtherActor))
	{
		return;
	}

	ActorsOverlappedAfterDash.Remove(OtherActor);

	if (ActorsOverlappedAfterDash.IsEmpty())
	{
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bUseRVOAvoidance = true;
		}
		UE_LOG(LogTemp, Display, TEXT("Every actor overlap after dash has been resolved."));
	}
}

void AAssassinsCharacter::HandleMoveSpeedChanged(float OldValue, float NewValue)
{
	GetCharacterMovement()->MaxWalkSpeed = NewValue;
}

void AAssassinsCharacter::OnChannelingTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		if (AAssassinsPlayerController* AssassinsPC = Cast<AAssassinsPlayerController>(GetController()))
		{
			AssassinsPC->PauseMove();
		}
		OnChannelingStarted.Broadcast();
	}
	else
	{
		// Rotation rate might have been changed by the ability.
		GetCharacterMovement()->RotationRate = FRotator(-1.0f, -1.0f, -1.0f);

		if (AAssassinsPlayerController* AssassinsPC = Cast<AAssassinsPlayerController>(GetController()))
		{
			AssassinsPC->ResumeMove();
		}
		OnChannelingEnded.Broadcast();
	}
}

void AAssassinsCharacter::OnUntargetableTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{ 
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
		}
	}
	else
	{
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		}
	}
}

void AAssassinsCharacter::OnInvisibleTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		OnInvisibilityStarted.Broadcast();
		if (GetLocalRole() == ROLE_SimulatedProxy)
		{
			SetActorHiddenInGame(true);
		}
	}
	else
	{
		OnInvisibilityEnded.Broadcast();
		if (GetLocalRole() == ROLE_SimulatedProxy)
		{
			SetActorHiddenInGame(false);
		}
	}
}

void AAssassinsCharacter::OnDashingTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	UAssassinsCharacterMovementComponent* AssassinsCharacterMovement = Cast<UAssassinsCharacterMovementComponent>(GetCharacterMovement());
	check(Capsule && AssassinsCharacterMovement);

	if (NewCount > 0)
	{
		AssassinsCharacterMovement->bIsDashing = 1;
		AssassinsCharacterMovement->bUseRVOAvoidance = false;
		Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	}
	else
	{
		AssassinsCharacterMovement->bIsDashing = 0;
		AssassinsCharacterMovement->RotationRate = FRotator(-1.0f, -1.0f, -1.0f);

		ResolvePenetrationAfterDash();
	}
}

void AAssassinsCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	if (AAssassinsPlayerController* AssassinsPC = Cast<AAssassinsPlayerController>(GetController()))
	{
		AssassinsPC->SetAvoidanceGroup(GenericTeamIdToInteger(MyTeamID));
	}
}