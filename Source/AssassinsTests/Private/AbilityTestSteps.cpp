// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilityTestSteps.h"
#include "AssassinsTestsLog.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"

namespace AbilityTestSteps
{
	// Runs the steps of a test, a frame at a time, until they are done.
	class FLatentCommand : public IAutomationLatentCommand
	{
	public:
		explicit FLatentCommand(TSharedRef<FAbilityTestSteps> InSteps)
			: Steps(MoveTemp(InSteps))
		{
		}

		virtual bool Update() override
		{
			return Steps->Update();
		}

	private:
		TSharedRef<FAbilityTestSteps> Steps;
	};
}

FAbilityTestSteps::FAbilityTestSteps(FAutomationTestBase& InTest)
	: Test(InTest)
{
}

FAbilityTestSteps& FAbilityTestSteps::Do(const FString& Description, TFunction<void()> Action)
{
	return Step(Description, DefaultTimeoutSeconds, [Action = MoveTemp(Action)](FString&)
	{
		Action();
		return EResult::Next;
	});
}

FAbilityTestSteps& FAbilityTestSteps::Step(const FString& Description, double TimeoutSeconds, FStepFunction Function)
{
	Steps.Add(FStep{ Description, TimeoutSeconds, MoveTemp(Function) });
	return *this;
}

FAbilityTestSteps& FAbilityTestSteps::WaitUntil(const FString& Description, double TimeoutSeconds, TFunction<bool()> Condition)
{
	return Step(Description, TimeoutSeconds, [Condition = MoveTemp(Condition)](FString&)
	{
		return Condition() ? EResult::Next : EResult::Wait;
	});
}

FAbilityTestSteps& FAbilityTestSteps::WaitFor(const FString& Description, double Seconds, TFunction<void()> EachFrame)
{
	TSharedRef<double> StartTime = MakeShared<double>(-1.0);
	return Step(Description, Seconds + 5.0, [StartTime, Seconds, EachFrame = MoveTemp(EachFrame)](FString&)
	{
		if (EachFrame)
		{
			EachFrame();
		}

		const double Now = FPlatformTime::Seconds();
		if (*StartTime < 0.0)
		{
			*StartTime = Now;
		}
		return (Now - *StartTime >= Seconds) ? EResult::Next : EResult::Wait;
	});
}

FAbilityTestSteps& FAbilityTestSteps::Finally(TFunction<void()> Action)
{
	FinallyActions.Add(MoveTemp(Action));
	return *this;
}

FAbilityTestSteps& FAbilityTestSteps::Require(TFunction<bool(FString& OutError)> Condition)
{
	Requirements.Add(FRequirement{ Steps.Num(), MoveTemp(Condition) });
	return *this;
}

FAbilityTestSteps& FAbilityTestSteps::EachFrame(TFunction<void()> Action)
{
	return Require([Action = MoveTemp(Action)](FString&)
	{
		Action();
		return true;
	});
}

void FAbilityTestSteps::Run()
{
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<AbilityTestSteps::FLatentCommand>(AsShared()));
}

bool FAbilityTestSteps::Update()
{
	if (bFinished)
	{
		return true;
	}

	if (!Steps.IsValidIndex(CurrentStep))
	{
		Finish();
		return true;
	}

	FStep& Current = Steps[CurrentStep];
	const double Now = FPlatformTime::Seconds();
	if (CurrentStepStartTime < 0.0)
	{
		CurrentStepStartTime = Now;
		LastWaitReason.Reset();
		UE_LOG(LogAssassinsTests, Log, TEXT("[%s] %s"), *Test.GetTestFullName(), *Current.Description);
	}

	const double ElapsedSeconds = Now - CurrentStepStartTime;

	FString Error;
	for (const FRequirement& Requirement : Requirements)
	{
		if ((Requirement.FirstStep <= CurrentStep) && !Requirement.Condition(Error))
		{
			Fail(Current, Error, ElapsedSeconds);
			return true;
		}
	}

	switch (Current.Function(Error))
	{
	case EResult::Next:
		++CurrentStep;
		CurrentStepStartTime = -1.0;
		if (!Steps.IsValidIndex(CurrentStep))
		{
			Finish();
			return true;
		}
		return false;

	case EResult::Fail:
		Fail(Current, Error, ElapsedSeconds);
		return true;

	case EResult::Wait:
	default:
		if (!Error.IsEmpty())
		{
			LastWaitReason = Error;
		}

		if (ElapsedSeconds > Current.TimeoutSeconds)
		{
			const FString TimeoutError = LastWaitReason.IsEmpty()
				? FString::Printf(TEXT("timed out after %.1fs"), Current.TimeoutSeconds)
				: FString::Printf(TEXT("timed out after %.1fs: %s"), Current.TimeoutSeconds, *LastWaitReason);
			Fail(Current, TimeoutError, ElapsedSeconds);
			return true;
		}
		return false;
	}
}

void FAbilityTestSteps::Fail(const FStep& Step, const FString& Error, double ElapsedSeconds)
{
	bFailed = true;
	Test.AddError(FString::Printf(TEXT("%s: %s (%.1fs into the step)"), *Step.Description, Error.IsEmpty() ? TEXT("failed") : *Error, ElapsedSeconds));
	Finish();
}

void FAbilityTestSteps::Finish()
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	for (TFunction<void()>& Action : FinallyActions)
	{
		Action();
	}
	FinallyActions.Reset();
}
