// Fill out your copyright notice in the Description page of Project Settings.

#include "AssassinsTestsLog.h"
#include "AbilityTestSession.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogAssassinsTests);

/**
 * FAssassinsTestsModule
 *
 * The automation tests of the game(Assassins.*). Loading the module only registers the tests: the play session and its
 * hooks come with the first test that needs them.
 */
class FAssassinsTestsModule : public IModuleInterface
{
public:
	virtual void ShutdownModule() override
	{
		FAbilityTestSession::Get().Shutdown();
	}
};

IMPLEMENT_MODULE(FAssassinsTestsModule, AssassinsTests);
