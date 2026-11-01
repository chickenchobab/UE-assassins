#pragma once

#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNodeReference.h"
#include "AssassinsZedShadowAnimInstance.generated.h"

UCLASS(Abstract)
class UAssassinsZedShadowAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Assassins|Animation", meta = (BlueprintThreadSafe))
	void RemoveMontageCompletionFlag(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Assassins|Animation")
	bool IsMontageEnded = false;
};
