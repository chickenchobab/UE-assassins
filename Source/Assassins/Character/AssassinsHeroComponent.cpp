// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AssassinsHeroComponent.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "Character/AssassinsPawnData.h"
#include "Character/AssassinsCharacter.h"
#include "Player/AssassinsPlayerState.h"
#include "Player/AssassinsPlayerController.h"
#include "Player/AssassinsLocalPlayer.h"
#include "AssassinsGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Input/AssassinsInputComponent.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "UI/AssassinsGameViewportClient.h"


const FName UAssassinsHeroComponent::NAME_ActorFeatureName("Hero");
const FName UAssassinsHeroComponent::NAME_BindInputsNow("BindInputsNow");

UAssassinsHeroComponent::UAssassinsHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;

    MoveBlockingStatusTags.AddTag(AssassinsGameplayTags::Status_Channeling);

    MoveInterruptedAbilityTags.AddTag(AssassinsGameplayTags::Ability_Interruptible_Click);
}

bool UAssassinsHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == AssassinsGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == AssassinsGameplayTags::InitState_Spawned && DesiredState == AssassinsGameplayTags::InitState_DataAvailable)
	{
		// The player state is required.
		if (!GetPlayerState<AAssassinsPlayerState>())
		{
			return false;
		}

		// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = \
				(Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			AAssassinsPlayerController* AssassinsPC = GetController<AAssassinsPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !AssassinsPC || !AssassinsPC->GetLocalPlayer())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == AssassinsGameplayTags::InitState_DataAvailable && DesiredState == AssassinsGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		AAssassinsPlayerState* AssassinsPS = GetPlayerState<AAssassinsPlayerState>();

		return AssassinsPS && Manager->HasFeatureReachedInitState(Pawn, UAssassinsPawnExtensionComponent::NAME_ActorFeatureName, AssassinsGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == AssassinsGameplayTags::InitState_DataInitialized && DesiredState == AssassinsGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void UAssassinsHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == AssassinsGameplayTags::InitState_DataAvailable && DesiredState == AssassinsGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		AAssassinsPlayerState* AssassinsPS = GetPlayerState<AAssassinsPlayerState>();
		if (!ensure(Pawn && AssassinsPS))
		{
			return;
		}

		const UAssassinsPawnData* PawnData = nullptr;

		if (UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnData = PawnExtComp->GetPawnData<UAssassinsPawnData>();

			// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
			// The ability system component and attribute sets live on the player state.
			PawnExtComp->InitializeAbilitySystem(AssassinsPS->GetAssassinsAbilitySystemComponent(), AssassinsPS);

            if (UAssassinsAbilitySystemComponent* AssassinsASC = AssassinsPS->GetAssassinsAbilitySystemComponent())
            {
                AssassinsASC->OnBeginChanneling.AddDynamic(this, &UAssassinsHeroComponent::HandleBeginChanneling);
                AssassinsASC->OnEndChanneling.AddDynamic(this, &UAssassinsHeroComponent::HandleEndChanneling);
            }
		}

		if (AAssassinsPlayerController* AssassinsPC = GetController<AAssassinsPlayerController>())
		{
			CachedPlayerController = AssassinsPC;

			CachedPlayerController->bShowMouseCursor = true;
			CachedPlayerController->DefaultMouseCursor = EMouseCursor::Default;

			if (Pawn->InputComponent)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}
	}
}

void UAssassinsHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UAssassinsPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == AssassinsGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void UAssassinsHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { AssassinsGameplayTags::InitState_Spawned, AssassinsGameplayTags::InitState_DataAvailable, AssassinsGameplayTags::InitState_DataInitialized, AssassinsGameplayTags::InitState_GameplayReady };

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}


void UAssassinsHeroComponent::OnRegister()
{
	Super::OnRegister();;

	// It should be added to a blueprint whose base class is a Pawn.
	if (GetPawn<APawn>()) 
	{
		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

void UAssassinsHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UAssassinsPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(AssassinsGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UAssassinsHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UAssassinsHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const UAssassinsLocalPlayer* LP = Cast<UAssassinsLocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UAssassinsPawnData* PawnData = PawnExtComp->GetPawnData<UAssassinsPawnData>())
		{
			if (UAssassinsInputConfig* InputConfig = PawnData->InputConfig)
			{
				// Add default Input Mapping Contexts
				for (const FInputMappingContextAndPriority Mapping : DefaultInputMappings)
				{
					if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
					{
						Subsystem->AddMappingContext(IMC, Mapping.Priority);
					}
				}

				// Set up action bindings
				if (UAssassinsInputComponent* AssassinsIC = Cast<UAssassinsInputComponent>(PlayerInputComponent))
				{
					// Setup mouse input events
					// Me: The tags are not used anymore
					AssassinsIC->BindNativeAction(InputConfig, AssassinsGameplayTags::InputTag_SetDestination_Click, ETriggerEvent::Started, this, &ThisClass::OnInputStarted);
					AssassinsIC->BindNativeAction(InputConfig, AssassinsGameplayTags::InputTag_SetDestination_Click, ETriggerEvent::Triggered, this, &ThisClass::OnSetDestinationTriggered);
					AssassinsIC->BindNativeAction(InputConfig, AssassinsGameplayTags::InputTag_SetDestination_Click, ETriggerEvent::Completed, this, &ThisClass::OnSetDestinationReleased);
					AssassinsIC->BindNativeAction(InputConfig, AssassinsGameplayTags::InputTag_SetDestination_Click, ETriggerEvent::Canceled, this, &ThisClass::OnSetDestinationReleased);

					TArray<uint32> BindHandles;
					AssassinsIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
				}
				else
				{
					UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
				}
			}
		}
	}

	UGameInstance* GameInstance = GetGameInstance<UGameInstance>();
	check(GameInstance);

	UAssassinsGameViewportClient* GameViewport = Cast<UAssassinsGameViewportClient>(GameInstance->GetGameViewportClient());
	check(GameViewport);

	GameViewport->OnCursorTargetSet.BindUObject(this, &ThisClass::HandleCursorTargetSet);
	GameViewport->OnCursorTargetCleared.BindUObject(this, &ThisClass::HandleCursorTargetCleared);

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UAssassinsHeroComponent::OnInputStarted()
{
    if (!CanMove())
    {
        return;
    }

    CancelMoveInterruptedAbilities();

    if (CachedPlayerController)
    {
        CachedPlayerController->StopMovement();
        CachedPlayerController->ResetMoveState();
    }
}

// Triggered every frame when the input is held down
void UAssassinsHeroComponent::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();

	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;

	if (CachedPlayerController)
	{
		bHitSuccessful = CachedPlayerController->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}

    if (!CanMove())
    {
        return;
    }

	// Move towards mouse pointer
	APawn* ControlledPawn = GetPawn<APawn>();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void UAssassinsHeroComponent::OnSetDestinationReleased()
{
    // Me: This function must not be called if the mouse cursor is pointing a target so 
    // dummy input action is added to override the action bound to this function whenever 
    // imput mapping contexts are switched due to the cursor.

	// If it was a short press
	if (FollowTime <= ShortPressThreshold)
	{
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(CachedPlayerController, CachedDestination);
        if (!CanMove())
        {
            if (CachedPlayerController)
            {
                // Me: Reserve movement 
                CachedPlayerController->PauseMove();
            }
        }
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

void UAssassinsHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* PlayerPawn = GetPawn<APawn>())
	{
		if (const UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(PlayerPawn))
		{
			if (UAssassinsAbilitySystemComponent* AssassinsASC = PawnExtComp->GetAssassinsAbilitySystemComponent())
			{
				AssassinsASC->AbilityInputTagPressed(InputTag);
			}
		}
	}
}

void UAssassinsHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (const APawn* PlayerPawn = GetPawn<APawn>())
	{
		if (const UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(PlayerPawn))
		{
			if (UAssassinsAbilitySystemComponent* AssassinsASC = PawnExtComp->GetAssassinsAbilitySystemComponent())
			{
				AssassinsASC->AbilityInputTagReleased(InputTag);
			}
		}
	}
}

void UAssassinsHeroComponent::HandleCursorTargetSet(AAssassinsCharacter* CursorTarget)
{
	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const UAssassinsLocalPlayer* LP = Cast<UAssassinsLocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	check(Subsystem);

	if (AttackInputMapping)
	{
		Subsystem->AddMappingContext(AttackInputMapping, 1);
	}

	if (APawn* PlayerPawn = GetPawn<APawn>())
	{
		if (AAssassinsCharacter* PlayerCharacter = Cast<AAssassinsCharacter>(PlayerPawn))
		{
			PlayerCharacter->SetAbilityTarget(CursorTarget);
		}
	}
}

void UAssassinsHeroComponent::HandleCursorTargetCleared()
{
	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const UAssassinsLocalPlayer* LP = Cast<UAssassinsLocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	check(Subsystem);

	if (AttackInputMapping)
	{
		Subsystem->RemoveMappingContext(AttackInputMapping);
	}

	if (APawn* PlayerPawn = GetPawn<APawn>())
	{
		if (AAssassinsCharacter* PlayerCharacter = Cast<AAssassinsCharacter>(PlayerPawn))
		{
			PlayerCharacter->ClearAbilityTarget();
		}
	}
}

bool UAssassinsHeroComponent::CanMove()
{
    if (const APawn* PlayerPawn = GetPawn<APawn>())
    {
        if (const UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(PlayerPawn))
        {
            if (UAssassinsAbilitySystemComponent* AssassinsASC = PawnExtComp->GetAssassinsAbilitySystemComponent())
            {
                return !AssassinsASC->HasAnyMatchingGameplayTags(MoveBlockingStatusTags);
            }
        }
    }

    return false;
}

void UAssassinsHeroComponent::CancelMoveInterruptedAbilities()
{
    if (const APawn* PlayerPawn = GetPawn<APawn>())
    {
        if (const UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(PlayerPawn))
        {
            if (UAssassinsAbilitySystemComponent* AssassinsASC = PawnExtComp->GetAssassinsAbilitySystemComponent())
            {
                AssassinsASC->CancelAbilities(&MoveInterruptedAbilityTags);
            }
        }
    }
}

void UAssassinsHeroComponent::HandleBeginChanneling()
{
    if (CachedPlayerController)
    {
        CachedPlayerController->HandleBeginChanneling();
    }
}

void UAssassinsHeroComponent::HandleEndChanneling(bool bContinuePaused)
{
    if (CachedPlayerController)
    {
        // Me: If movement is neither paused nor reserved, allow the ability montage to play to completion as normal
        /*if (CachedPlayerController->HasMovePaused())
        {
            CancelMoveInterruptedAbilities();
        }*/

        CachedPlayerController->HandleEndChanneling(bContinuePaused);
    }
}
