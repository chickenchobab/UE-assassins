#include "Character/AssassinsPredictedActor.h"

#include "GameFramework/Pawn.h"

bool AAssassinsPredictedActor::ShouldSkipCosmetic() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return !HasAuthority() && OwnerPawn && OwnerPawn->IsLocallyControlled();
}
