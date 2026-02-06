// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "CommandDeckCoreModule.h"
#include "UiCommandRegistry.h"

DEFINE_LOG_CATEGORY(LogCommandDeckCore);

#define LOCTEXT_NAMESPACE "FCommandDeckCoreModule"

void FCommandDeckCoreModule::StartupModule()
{
	FUiCommandRegistry::Get().Startup();
}

void FCommandDeckCoreModule::ShutdownModule()
{
	FUiCommandRegistry::Get().Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCommandDeckCoreModule, CommandDeckCore)