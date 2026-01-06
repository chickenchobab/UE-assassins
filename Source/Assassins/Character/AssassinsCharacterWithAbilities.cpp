// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AssassinsCharacterWithAbilities.h"
#include "Character/AssassinsHealthComponent.h"
#include "AbilitySystem/AssassinsAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/AssassinsHealthSet.h"
#include "AbilitySystem/Attributes/AssassinsCombatSet.h"
#include "AbilitySystem/AssassinsAbilitySet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Bot/AssassinsBotController.h"


AAssassinsCharacterWithAbilities::AAssassinsCharacterWithAbilities()
{
    AbilitySystemComponent = CreateDefaultSubobject<UAssassinsAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

    // These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
    HealthSet = CreateDefaultSubobject<UAssassinsHealthSet>(TEXT("HealthSet"));
    CombatSet = CreateDefaultSubobject<UAssassinsCombatSet>(TEXT("CombatSet"));
}

void AAssassinsCharacterWithAbilities::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    check(AbilitySystemComponent);
    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    UAssassinsHealthComponent::FindHealthComponent(this)->InitializeWithAbilitySystem(AbilitySystemComponent);

    if (CombatSet)
    {
        GetCharacterMovement()->MaxWalkSpeed = CombatSet->GetMoveSpeed();
        // Me: GetSet returns const pointer but member delegates are set mutable
        CombatSet->OnMoveSpeedChanged.AddUObject(this, &AAssassinsCharacterWithAbilities::HandleMoveSpeedChanged);
    }

    // Me: Apply ability set which cannot be granted by pawn data
    if (AbilitySet)
    {
        FAssassinsAbilitySet_GrantedHandles Handles;
        AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &Handles);
    }
}

UAbilitySystemComponent* AAssassinsCharacterWithAbilities::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AAssassinsCharacterWithAbilities::SetGenericTeamId(const FGenericTeamId& NewID)
{
    if (AAssassinsBotController* BotController = GetController<AAssassinsBotController>())
    {
        if (BotController->CanSetPawnTeamId())
        {
            SetTeamId(NewID);
            return;
        }
    }

    Super::SetGenericTeamId(NewID);
}
