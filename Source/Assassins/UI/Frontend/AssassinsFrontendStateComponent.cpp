// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Frontend/AssassinsFrontendStateComponent.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "ControlFlowManager.h"
#include "Kismet/GameplayStatics.h"
#include "CommonUserSubsystem.h"
#include "CommonSessionSubsystem.h"
#include "PrimaryGameLayout.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");

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
    if (bShouldShowLoadingScreen)
    {
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
    return false;
}

void UAssassinsFrontendStateComponent::OnExperienceLoaded(const UAssassinsExperienceDefinition* Experience)
{
    FControlFlow& Flow = FControlFlowStatics::Create(this, TEXT("FrontendFlow"))
        .QueueStep(TEXT("Wait For User Initialization"), this, &ThisClass::FlowStep_WaitForUserInitialization)
        .QueueStep(TEXT("Try Initialize User"), this, &ThisClass::FlowStep_TryInitializeUser)
        .QueueStep(TEXT("Try Show Main Screen"), this, &ThisClass::FlowStrp_TryShowMainScreen);

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

    // Me: TODO Reset sessions

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
    
    if (ensure(FlowToContinue.IsValid()))
    {
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

void UAssassinsFrontendStateComponent::FlowStrp_TryShowMainScreen(FControlFlowNodeRef SubFlow)
{
    if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this))
    {
        constexpr bool bSuspendInputUntilComplete = true;
        RootLayout->PushWidgetToLayerStackAsync<UCommonActivatableWidget>(TAG_UI_LAYER_MENU, bSuspendInputUntilComplete, MainScreenClass,
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