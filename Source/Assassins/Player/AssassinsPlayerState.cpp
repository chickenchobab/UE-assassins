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


const FName AAssassinsPlayerState::NAME_AssassinsAbilityReady("AssassinsAbilityReady");

AAssassinsPlayerState::AAssassinsPlayerState(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UAssassinsAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));

    // These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
    HealthSet = CreateDefaultSubobject<UAssassinsHealthSet>(TEXT("HealthSet"));
    CombatSet = CreateDefaultSubobject<UAssassinsCombatSet>(TEXT("CombatSet"));
}

AAssassinsPlayerController* AAssassinsPlayerState::GetAssassinsPlayerController() const
{
    return Cast<AAssassinsPlayerController>(GetOwner());
}

UAbilitySystemComponent* AAssassinsPlayerState::GetAbilitySystemComponent() const
{ 
    return GetAssassinsAbilitySystemComponent();
}

void AAssassinsPlayerState::SetPawnData(const UAssassinsPawnData* InPawnData)
{
    check(InPawnData);

    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    //MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);

    PawnData = InPawnData;

    ApplyAbilitySets();
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

void AAssassinsPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    if (AAssassinsPlayerState* AssassinsPS = Cast<AAssassinsPlayerState>(PlayerState))
    {
        AssassinsPS->SelectedExperience = SelectedExperience;
        AssassinsPS->MyTeamID = MyTeamID;
        AssassinsPS->PawnData = PawnData;
    }
}

void AAssassinsPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    if (HasAuthority())
    {
        const FGenericTeamId OldTeamID = MyTeamID;
        MyTeamID = NewTeamID;

        if (AAssassinsPlayerController* AssassinsPC = GetAssassinsPlayerController())
        {
            AssassinsPC->SetAvoidanceGroup(GenericTeamIdToInteger(NewTeamID));
        }
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