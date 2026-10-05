// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssassinsCharacter.h"

#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "UObject/ConstructorHelpers.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Character/AssassinsHealthComponent.h"
#include "Character/Movements/AssassinsCharacterMovementComponent.h"
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

namespace CharacterStatus
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_CHANNELING, "Status.Channeling");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_UNTARGETABLE, "Status.Untargetable");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_INVISIBLE, "Status.Untargetable.Invisible");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_DASHING, "Status.Dashing");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_STATUS_ROOTED, "Status.Rooted");
};

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
	ResetRotationRate();
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 50.0f;

	GetCharacterMovement()->MaxAcceleration = 100000.f; // Me: For instant attainment of the specified speed(combat set)
	GetCharacterMovement()->GetNavMovementProperties()->bUseFixedBrakingDistanceForPaths = true;
	GetCharacterMovement()->GetNavMovementProperties()->FixedPathBrakingDistance = 0; // Me: For instant stop after navigation

	GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = false;

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

void AAssassinsCharacter::HandleGenericGameplayTagEvent_Implementation(const FGameplayTag Tag, int32 NewCount)
{
}

void AAssassinsCharacter::InitializeGameplayTags()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTags(CharacterOwnedTags);

		ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &ThisClass::HandleGenericGameplayTagEvent);

		ASC->RegisterGameplayTagEvent(CharacterStatus::TAG_STATUS_CHANNELING).AddUObject(this, &ThisClass::OnChannelingTagChanged);
		ASC->RegisterGameplayTagEvent(CharacterStatus::TAG_STATUS_UNTARGETABLE).AddUObject(this, &ThisClass::OnUntargetableTagChanged);
		ASC->RegisterGameplayTagEvent(CharacterStatus::TAG_STATUS_INVISIBLE).AddUObject(this, &ThisClass::OnInvisibleTagChanged);
		ASC->RegisterGameplayTagEvent(CharacterStatus::TAG_STATUS_DASHING).AddUObject(this, &ThisClass::OnDashingTagChanged);
		ASC->RegisterGameplayTagEvent(CharacterStatus::TAG_STATUS_ROOTED).AddUObject(this, &ThisClass::OnRootedTagChanged);
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

void AAssassinsCharacter::FinishDashMovement()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	check(Capsule);

	// The dash went through world static(OnDashingTagChanged).
	Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);

	if (!GetWorld()->OverlapBlockingTestByChannel(Capsule->GetComponentLocation(), Capsule->GetComponentQuat(), ECC_Pawn, Capsule->GetCollisionShape(), QueryParams, ResponseParams))
	{
		return;
	}

	// A sweep that starts inside tells the shortest way out, whichever way it goes, and whatever it is inside of.
	FHitResult Hit;
	GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(), GetActorLocation() - Capsule->GetScaledCapsuleRadius() * GetActorForwardVector(), Capsule->GetComponentQuat(), ECC_Pawn, Capsule->GetCollisionShape(), QueryParams, ResponseParams);
	if (!Hit.bStartPenetrating)
	{
		return;
	}

	UMovementComponent* Movement = GetMovementComponent();
	const FVector RequestedAdjustment = Movement->GetPenetrationAdjustment(Hit);
	if (!Movement->ResolvePenetration(RequestedAdjustment, Hit, Capsule->GetComponentQuat()))
	{
		// Teleport if the penetration has not been resolved.
		FVector TeleportLocation = GetActorLocation();
		if (GetWorld()->FindTeleportSpot(this, TeleportLocation, GetActorRotation()))
		{
			SetActorLocation(TeleportLocation);
		}
	}
}

void AAssassinsCharacter::WatchPawnOverlapsAfterDash()
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	check(Capsule);

	StopWatchingPawnOverlapsAfterDash();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Overlap);
	GetWorld()->OverlapMultiByChannel(Overlaps, Capsule->GetComponentLocation(), Capsule->GetComponentQuat(), ECC_Pawn, Capsule->GetCollisionShape(), QueryParams, ResponseParams);

	for (const FOverlapResult& Result : Overlaps)
	{
		if (AActor* OverlappedActor = Result.GetActor())
		{
			ActorsOverlappedAfterDash.Add(OverlappedActor);
			UE_LOG(LogTemp, Display, TEXT("Found overlapping pawn : [%s]"), *OverlappedActor->GetName());
		}
	}

	if (ActorsOverlappedAfterDash.IsEmpty())
	{
		GetCharacterMovement()->bUseRVOAvoidance = true;
		return;
	}

	Capsule->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnEndPawnOverlapAfterDash);
}

void AAssassinsCharacter::StopWatchingPawnOverlapsAfterDash()
{
	ActorsOverlappedAfterDash.Reset();

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->OnComponentEndOverlap.RemoveDynamic(this, &ThisClass::OnEndPawnOverlapAfterDash);
	}
}

void AAssassinsCharacter::OnEndPawnOverlapAfterDash(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ActorsOverlappedAfterDash.Remove(OtherActor) == 0)
	{
		return;
	}

	if (ActorsOverlappedAfterDash.IsEmpty())
	{
		StopWatchingPawnOverlapsAfterDash();
		GetCharacterMovement()->bUseRVOAvoidance = true;
		UE_LOG(LogTemp, Display, TEXT("Every actor overlap after dash has been resolved."));
	}
}

void AAssassinsCharacter::HandleMoveSpeedChanged(float OldValue, float NewValue)
{
	GetCharacterMovement()->MaxWalkSpeed = NewValue;
}

void AAssassinsCharacter::FreezeRotation()
{
	GetCharacterMovement()->RotationRate = FRotator::ZeroRotator;
}

void AAssassinsCharacter::ResetRotationRate()
{
	// Below zero the movement turns the character at once(UCharacterMovementComponent::GetDeltaRotation).
	GetCharacterMovement()->RotationRate = FRotator(-1.0, -1.0, -1.0);
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
		// The ability may have frozen the rotation(FreezeRotation).
		ResetRotationRate();

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
		if (!IsLocallyControlled())
		{
			GetRootComponent()->SetVisibility(false, true);
		}
	}
	else
	{
		OnInvisibilityEnded.Broadcast();
		if (!IsLocallyControlled())
		{
			GetRootComponent()->SetVisibility(true, true);
			GetRootComponent()->SetVisibility(false, false);
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

		// The pawns an earlier dash left the character on no longer count: the end of their overlap must not turn the
		// avoidance back on during this dash.
		StopWatchingPawnOverlapsAfterDash();
	}
	else
	{
		AssassinsCharacterMovement->bIsDashing = 0;
		ResetRotationRate();

		FinishDashMovement();
		WatchPawnOverlapsAfterDash();
	}
}

void AAssassinsCharacter::OnRootedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		if (GetController())
		{
			GetController()->SetIgnoreMoveInput(true);
		}

		UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
		check(MovementComponent);
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}
	else
	{
		if (GetController())
		{
			GetController()->SetIgnoreMoveInput(false);
		}

		UAssassinsCharacterMovementComponent* AssassinsCMC = Cast<UAssassinsCharacterMovementComponent>(GetCharacterMovement());
		check(AssassinsCMC);
		AssassinsCMC->EnableMovement();
	}
}

void AAssassinsCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	if (AAssassinsPlayerController* AssassinsPC = Cast<AAssassinsPlayerController>(GetController()))
	{
		AssassinsPC->SetAvoidanceGroup(GenericTeamIdToInteger(MyTeamID));
	}
}