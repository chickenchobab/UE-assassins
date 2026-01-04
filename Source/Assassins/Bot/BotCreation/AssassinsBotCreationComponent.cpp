// Fill out your copyright notice in the Description page of Project Settings.


#include "Bot/BotCreation/AssassinsBotCreationComponent.h"
#include "GameModes/AssassinsExperienceStateComponent.h"
#include "AIController.h"

UAssassinsBotCreationComponent::UAssassinsBotCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAssassinsBotCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the experience load to complete
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	UAssassinsExperienceStateComponent* ExperienceComponent = GameState->FindComponentByClass<UAssassinsExperienceStateComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_LowPriority(FOnAssassinsExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void UAssassinsBotCreationComponent::OnExperienceLoaded(const UAssassinsExperienceDefinition* Experience)
{
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateBots();
	}
#endif
}

#if WITH_SERVER_CODE

void UAssassinsBotCreationComponent::ServerCreateBots_Implementation()
{

}

void UAssassinsBotCreationComponent::SpawnOneBot()
{

}

void UAssassinsBotCreationComponent::RemoveOneBot(AAIController* Controller)
{
	
}


#else // !WITH_SERVER_CODE

void UAssassinsBotCreationComponent::ServerCreateBots()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in Assassins client!"))
}

void UAssassinsBotCreationComponent::SpawnOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in Assassins client!"))
}

void UAssassinsBotCreationComponent::RemoveOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in Assassins client!"))
}

#endif