#include "AbilitySystem/Abilities/Death/Ability_Nexus_Death.h"

#include "Teams/AssassinsTeamBaseActor.h"

void UAbility_Nexus_Death::OnMontageEnd()
{
	if (AAssassinsTeamBaseActor* Nexus = Cast<AAssassinsTeamBaseActor>(GetAvatarActorFromActorInfo()))
	{
		Nexus->ReturnAllPlayersToFrontend();
	}

	Super::OnMontageEnd();
}
