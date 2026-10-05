// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

class FAutomationTestBase;

/**
 * FAbilityTestSteps
 *
 * A test as a list of steps that run one after the other over frames, at most one a frame. A step does something, then
 * says whether to go on, keep waiting(up to its time limit) or fail. Failing skips the rest of the steps, but not the
 * ones added with Finally, which always run at the end.
 */
class FAbilityTestSteps : public TSharedFromThis<FAbilityTestSteps>
{
public:
	enum class EResult : uint8
	{
		Next,
		Wait,
		Fail
	};

	// Fills the error when it fails. While waiting, what it fills is what the time limit reports.
	using FStepFunction = TFunction<EResult(FString& OutError)>;

	static constexpr double DefaultTimeoutSeconds = 10.0;

	explicit FAbilityTestSteps(FAutomationTestBase& InTest);

	// Runs the action once and goes on.
	FAbilityTestSteps& Do(const FString& Description, TFunction<void()> Action);

	// Runs the function every frame until it goes on or fails.
	FAbilityTestSteps& Step(const FString& Description, double TimeoutSeconds, FStepFunction Function);

	// Waits until the condition holds.
	FAbilityTestSteps& WaitUntil(const FString& Description, double TimeoutSeconds, TFunction<bool()> Condition);

	// Lets the time pass, running EachFrame(when given) every frame meanwhile.
	FAbilityTestSteps& WaitFor(const FString& Description, double Seconds, TFunction<void()> EachFrame = nullptr);

	// Runs at the end, whether the steps went through or not.
	FAbilityTestSteps& Finally(TFunction<void()> Action);

	// Checked before each step added after it: the step fails when it does not hold(e.g. the play session went away).
	FAbilityTestSteps& Require(TFunction<bool(FString& OutError)> Condition);

	// Runs every frame before each step added after it, e.g. to watch what the checks look back on.
	FAbilityTestSteps& EachFrame(TFunction<void()> Action);

	// Hands the steps to the test as a latent command.
	void Run();

	// Advances the current step. True once every step ran, or one failed.
	bool Update();

	bool HasFailed() const { return bFailed; }

	FAutomationTestBase& GetTest() const { return Test; }

private:
	struct FStep
	{
		FString Description;
		double TimeoutSeconds = DefaultTimeoutSeconds;
		FStepFunction Function;
	};

	void Fail(const FStep& Step, const FString& Error, double ElapsedSeconds);
	void Finish();

	struct FRequirement
	{
		int32 FirstStep = 0;
		TFunction<bool(FString& OutError)> Condition;
	};

	FAutomationTestBase& Test;
	TArray<FStep> Steps;
	TArray<FRequirement> Requirements;
	TArray<TFunction<void()>> FinallyActions;

	int32 CurrentStep = 0;
	double CurrentStepStartTime = -1.0;
	FString LastWaitReason;
	bool bFailed = false;
	bool bFinished = false;
};
