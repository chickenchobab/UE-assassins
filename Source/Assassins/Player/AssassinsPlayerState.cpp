// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AssassinsPlayerState.h"
#include "Player/AssassinsPlayerController.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/AssassinsAbilitySet.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "GameModes/AssassinsGameMode.h"
#include "GameModes/AssassinsGameState.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "Character/AssassinsPawnData.h"
#include "Character/AssassinsPawnExtensionComponent.h"
#include "AssassinsLogCategories.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Net/UnrealNetwork.h"

const FName AAssassinsPlayerState::NAME_AssassinsAbilityReady("AssassinsAbilityReady");

AAssassinsPlayerState::AAssassinsPlayerState(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UAssassinsAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    // These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
    HealthSet = CreateDefaultSubobject<UAssassinsHealthSet>(TEXT("HealthSet"));
    CombatSet = CreateDefaultSubobject<UAssassinsCombatSet>(TEXT("CombatSet"));

    // AbilitySystemComponent needs to be updated at a high frequency.
    SetNetUpdateFrequency(100.0f);
    
    MyTeamID = FGenericTeamId::NoTeam;

    bChampionSelected = false;
}

AAssassinsPlayerController* AAssassinsPlayerState::GetAssassinsPlayerController() const
{
    return Cast<AAssassinsPlayerController>(GetOwner());
}

UAbilitySystemComponent* AAssassinsPlayerState::GetAbilitySystemComponent() const
{ 
    return GetAssassinsAbilitySystemComponent();
}

void AAssassinsPlayerState::OnChampionSelected(const UAssassinsPawnData* InPawnData)
{
    if (APlayerController* PC = GetPlayerController())
    {
        if (PC->IsLocalPlayerController())
        {
            bChampionSelected = true;

            SetPawnData(InPawnData, false);

            Server_OnChampionSelected(InPawnData);
        }
    }
}

void AAssassinsPlayerState::Server_OnChampionSelected_Implementation(const UAssassinsPawnData* InPawnData)
{
    const ENetMode NetMode = GetNetMode();
    const bool bAlreadySetFlag = (NetMode == ENetMode::NM_Standalone || NetMode == ENetMode::NM_ListenServer);

    if (!bChampionSelected || bAlreadySetFlag)
    {
        SetPawnData(InPawnData, false);

        if (AAssassinsGameMode* GameMode = Cast<AAssassinsGameMode>(GetWorld()->GetAuthGameMode()))
        {
            // Broadcast that the champion has been selected(not available).
            GameMode->IncreaseReadyPlayers(InPawnData);
        }
    }
    
    bChampionSelected = true;
}

bool AAssassinsPlayerState::Server_OnChampionSelected_Validate(const UAssassinsPawnData* InPawnData)
{
    return (InPawnData != nullptr);
}

void AAssassinsPlayerState::SetPawnData(const UAssassinsPawnData* InPawnData, bool bShouldApplyAbilitySets)
{
    check(InPawnData);

    PawnData = InPawnData;

    if (bShouldApplyAbilitySets)
    {
        ApplyAbilitySets();
    }
}

void AAssassinsPlayerState::ApplyAbilitySets()
{
    if (PawnData)
    {
        for (UAssassinsAbilitySet* AbilitySet : PawnData->AbilitySets)
        {
            if (AbilitySet)
            {
                // Me: Shouldn't handles be needed to remove abilities?
                AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
            }
        }

        UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_AssassinsAbilityReady);
    }
}

void AAssassinsPlayerState::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    check(AbilitySystemComponent);
 
    AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());

    //UWorld* World = GetWorld();
    //if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
    //{
    //    AGameStateBase* GameState = World->GetGameState();
    //    check(GameState);
    //    UAssassinsExperienceStateComponent* ExperienceComponent = GameState->FindComponentByClass<UAssassinsExperienceStateComponent>();
    //    check(ExperienceComponent);
    //    ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnAssassinsExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
    //}
}

void AAssassinsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams SharedParams;
    SharedParams.bIsPushBased = true;

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);
}

void AAssassinsPlayerState::ClientInitialize(AController* C)
{
    Super::ClientInitialize(C);

    if (UAssassinsPawnExtensionComponent* PawnExtComp = UAssassinsPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
    {
        PawnExtComp->CheckDefaultInitialization();
    }
}

void AAssassinsPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    // Server travel occurs after lobby
    if (AAssassinsPlayerState* AssassinsPS = Cast<AAssassinsPlayerState>(PlayerState))
    {
        AssassinsPS->SetPawnData(PawnData, false);
    }
}

void AAssassinsPlayerState::OnDeactivated()
{
    if (bChampionSelected)
    {
        bChampionSelected = false;

        // Because this function is called on logout of the player, the game mode
        // should be notified of the logout
        if (AAssassinsGameMode* GameMode = Cast<AAssassinsGameMode>(GetWorld()->GetAuthGameMode()))
        {
            GameMode->DecreaseReadyPlayers(PawnData);
        }
    }

    Super::OnDeactivated();
}

void AAssassinsPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    if (HasAuthority())
    {
        const FGenericTeamId OldTeamID = MyTeamID;
        MyTeamID = NewTeamID;
    }
}

FGenericTeamId AAssassinsPlayerState::GetGenericTeamId() const
{
    return MyTeamID;
}

void AAssassinsPlayerState::OnExperienceLoaded(const UAssassinsExperienceDefinition* CurrentExperience)
{
    // Me: Pawn data for a champion bot is set later.
    if (IsABot())
    {
        return;
    }

    if (AAssassinsGameMode* AssassinsGameMode = GetWorld()->GetAuthGameMode<AAssassinsGameMode>())
    {
        // Me: Check whether this player already has PawnData; otherwise, retrieve the default from the loaded Experience.
        if (const UAssassinsPawnData* NewPawnData = AssassinsGameMode->GetPawnDataForController(GetOwningController()))
        {
            SetPawnData(NewPawnData);
        }
        else
        {
            UE_LOG(LogAssassins, Error, TEXT("AAssassinsPlayerState::OnExperienceLoaded(): Unable to find PawnData to initialize player state [%s]!"), *GetNameSafe(this));
        }
    }
}

void AAssassinsPlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
    if (AAssassinsPlayerController* AssassinsPC = Cast<AAssassinsPlayerController>(GetPlayerController()))
    {
        AssassinsPC->SetAvoidanceGroup(GenericTeamIdToInteger(MyTeamID));
    }
}
