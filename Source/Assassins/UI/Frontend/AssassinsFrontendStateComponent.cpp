// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Frontend/AssassinsFrontendStateComponent.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "GameModes/AssassinsGameState.h"
#include "ControlFlowManager.h"
#include "Kismet/GameplayStatics.h"
#include "CommonUserSubsystem.h"
#include "CommonSessionSubsystem.h"
#include "PrimaryGameLayout.h"
#include "NativeGameplayTags.h"
#include "System/AssassinsGameInstance.h"
#include "Player/AssassinsPlayerState.h"
#include "Player/AssassinsLocalPlayer.h"
#include "AssassinsLogCategories.h"

namespace FrontendTags
{
    UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");
}

UAssassinsFrontendStateComponent::UAssassinsFrontendStateComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UAssassinsFrontendStateComponent::BeginPlay()
{
    Super::BeginPlay();

    // Listen for the experience load to complete
    AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
    UAssassinsExperienceStateComponent* ExperienceComponent = GameState->FindComponentByClass<UAssassinsExperienceStateComponent>();
    check(ExperienceComponent);
    // This delegate is on a component with the same lifetime as this one, so no need to unhook it in.
    ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(FOnAssassinsExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

bool UAssassinsFrontendStateComponent::ShouldShowLoadingScreen(FString& OutReason) const
{
    if (!bShouldShowLoadingScreen)
    {
        return false;
    }

    AAssassinsGameState* GameState = GetGameStateChecked<AAssassinsGameState>();
    if (GameState->IsGameInLobby())
    {
        OutReason = TEXT("Champion Selection Loading...");
        return true;
    }
    
    OutReason = TEXT("Frontend Flow Pending...");
    if (FrontendFlow.IsValid())
    {
        const TOptional<FString> StepDebugName = FrontendFlow->GetCurrentStepDebugName();
        if (StepDebugName.IsSet())
        {
            OutReason = StepDebugName.GetValue();
        }
    }

    return true;
}

void UAssassinsFrontendStateComponent::CallOrRegister_ShowChampionSelectionScreen()
{
    if (bShouldRegisterShowChampionSelection)
    {
        bShouldRegisterShowChampionSelection = false;

        UAssassinsLocalPlayer* LocalPlayer = Cast<UAssassinsLocalPlayer>(GetWorld()->GetFirstLocalPlayerFromController());
        check(LocalPlayer);
        LocalPlayer->CallOrRegister_OnLocalPlayerRestarted(FLocalCharacterRestartedDelegate::FDelegate::CreateWeakLambda(this, [this](ACharacter*) {
            TryShowChampionSelectionScreen();
        }));
    }
}

void UAssassinsFrontendStateComponent::OnExperienceLoaded(const UAssassinsExperienceDefinition* Experience)
{
    AAssassinsGameState* GameState = GetGameStateChecked<AAssassinsGameState>();
    if (GameState->IsGameInLobby())
    {
        // Make sure the champion selection screen is shown on the player pawn restarted
        CallOrRegister_ShowChampionSelectionScreen();
        return; // User has already been initialized.
    }

    FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("FrontendFlow"))
        .QueueStep(TEXT("Wait For User Initialization"), this, &ThisClass::FlowStep_WaitForUserInitialization)
        .QueueStep(TEXT("Try Initialize User"), this, &ThisClass::FlowStep_TryInitializeUser)
        .QueueStep(TEXT("Try Show Main Screen"), this, &ThisClass::FlowStep_TryShowMainScreen);

    Flow.ExecuteFlow();
    
    FrontendFlow = Flow.AsShared();
}

void UAssassinsFrontendStateComponent::FlowStep_WaitForUserInitialization(FControlFlowNodeRef SubFlow)
{
    // If this was a hard disconnect, explicitly destroy all user and session state
    // TODO: Refactor the engine disconnect flow so it is more explicit about why it happend
    bool bWasHardDisconnect = false;
    AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>();
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);

    if (ensure(GameMode) && UGameplayStatics::HasOption(GameMode->OptionsString, TEXT("closed")))
    {
        bWasHardDisconnect = true;
    }

    // Only reset users on hard disconnect
    UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
    if (ensure(UserSubsystem) && bWasHardDisconnect)
    {
        UserSubsystem->ResetUserState();
    }

    // Always reset sessions
    UCommonSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UCommonSessionSubsystem>();
    if (ensure(SessionSubsystem))
    {
        SessionSubsystem->CleanUpSessions();
    }

    SubFlow->ContinueFlow();
}

void UAssassinsFrontendStateComponent::FlowStep_TryInitializeUser(FControlFlowNodeRef SubFlow)
{
    const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem< UCommonUserSubsystem>();
    
    // Check to see if the first player is already logged in, if they are, we can skip the press start screen.
    if (const UCommonUserInfo* FirstUser = UserSubsystem->GetUserInfoForLocalPlayerIndex(0))
    {
        if (FirstUser->InitializationState == ECommonUserInitializationState::LoggedInLocalOnly ||
            FirstUser->InitializationState == ECommonUserInitializationState::LoggedInOnline)
        {
            SubFlow->ContinueFlow();
            return;
        }
    }

    // Me: Skip the 'Press Start' screen for platforms where there can be multiple local users
    // and which player's controller pressess 'Start' establishes the player to actually login to the game.
    if (UserSubsystem->ShouldWaitForStartInput()) {}

    // Start the auto login process, this should finish quickly and will use the default input device id
    InProgressPressStartScreen = SubFlow;
    UserSubsystem->OnUserInitializeComplete.AddDynamic(this, &UAssassinsFrontendStateComponent::OnUserInitialized);
    UserSubsystem->TryToInitializeForLocalPlay(0, FInputDeviceId(), false);
}

void UAssassinsFrontendStateComponent::OnUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
    FControlFlowNodePtr FlowToContinue = InProgressPressStartScreen;
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UCommonUserSubsystem* UserSubsystem = GameInstance->GetSubsystem<UCommonUserSubsystem>();
    
    if (ensure(FlowToContinue.IsValid() && UserSubsystem))
    {
        UserSubsystem->OnUserInitializeComplete.RemoveDynamic(this, &UAssassinsFrontendStateComponent::OnUserInitialized);
        InProgressPressStartScreen.Reset();

        if (bSuccess)
        {
            FlowToContinue->ContinueFlow();
        }
        else
        {
            // TODO: Just continue for now, could go to some sort of error screen
            FlowToContinue->ContinueFlow();
        }
    }
}

void UAssassinsFrontendStateComponent::FlowStep_TryShowMainScreen(FControlFlowNodeRef SubFlow)
{
    if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
    {
        constexpr bool bSuspendInputUntilComplete = true;
        RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(FrontendTags::TAG_UI_LAYER_MENU, bSuspendInputUntilComplete, MainScreenClass,
            [this, SubFlow](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen) {
            switch (State)
            {
            case EAsyncWidgetLayerState::AfterPush:
                bShouldShowLoadingScreen = false;
                SubFlow->ContinueFlow();
                return;
            case EAsyncWidgetLayerState::Canceled:
                bShouldShowLoadingScreen = false;
                SubFlow->ContinueFlow();
                return;
            }
        });
    }
}

void UAssassinsFrontendStateComponent::TryShowChampionSelectionScreen()
{
    if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
    {
        constexpr bool bSuspendInputUntilComplete = true;
        RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(
            FrontendTags::TAG_UI_LAYER_MENU,
            bSuspendInputUntilComplete,
            ChampionSelectionScreenClass,
            [this](EAsyncWidgetLayerState State, UCommonActivatableWidget* Screen) {
                if (State == EAsyncWidgetLayerState::AfterPush)
                {
                    ChampionSelectionScreenWidget = Screen;
                    bShouldShowLoadingScreen = false;
                }
                else if (State == EAsyncWidgetLayerState::Canceled)
                {
                    bShouldShowLoadingScreen = false;
                }
            }
        );
    }
}
