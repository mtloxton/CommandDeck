// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCommandDeck, Warning, All);

class ICommandDeckPlugin;
class ICommandDeckConnection;

class COMMANDDECK_API FCommandDeckModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

	static FCommandDeckModule* Get() { return nullptr; }

	static ICommandDeckPlugin* GetPlugin() { return Plugin; }

	static ICommandDeckConnection* GetConnection() { return Connection; }

private:
	static ICommandDeckPlugin* Plugin;

	static ICommandDeckConnection* Connection;
};
