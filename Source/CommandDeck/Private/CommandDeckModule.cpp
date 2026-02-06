// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "CommandDeckModule.h"

#include "CommandDeckConnection.h"
#include "CommandDeckEditorObserver.h"
#include "CommandDeckPlugin.h"
#include "CommandDeckSettings.h"
#include "Misc/CoreDelegates.h"
#include "TriggerBlueprintAction/TriggerBlueprintActionManager.h"

DEFINE_LOG_CATEGORY(LogCommandDeck);

#define LOCTEXT_NAMESPACE "FCommandDeckModule"

ICommandDeckPlugin* FCommandDeckModule::Plugin = nullptr;

ICommandDeckConnection* FCommandDeckModule::Connection = nullptr;

void FCommandDeckModule::StartupModule()
{
	Connection = new FCommandDeckConnection();

	Plugin = new FCommandDeckPlugin(Connection);

	FTriggerBlueprintActionManager::Get();

	FCoreDelegates::OnPostEngineInit.AddLambda([this]()
		{
			const UCommandDeckSettings* Settings = GetDefault<UCommandDeckSettings>();
			if (Settings->bAutoConnect)
			{
				Connection->Connect();
			}

			FCommandDeckEditorObserver::Get().Startup();
		});
}

void FCommandDeckModule::ShutdownModule()
{
	FCommandDeckEditorObserver::Get().Shutdown();

	if (Plugin != nullptr)
	{
		delete Plugin;
		Plugin = nullptr;
	}

	if (Connection != nullptr)
	{
		delete Connection;
		Connection = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCommandDeckModule, CommandDeck)