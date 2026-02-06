// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "ICommandDeckPlugin.h"

class ICommandDeckConnection;

DECLARE_DELEGATE_OneParam(CommandDeckEventDelegate, TSharedPtr<FJsonObject> /*EventArgs*/);

class FCommandDeckPlugin : public ICommandDeckPlugin
{
public:
	FCommandDeckPlugin(ICommandDeckConnection* InConnection);

	virtual ~FCommandDeckPlugin();

	virtual void SendToApp(const FCommandDeckAction& InAction, FString InMessage) override;

	virtual void SendToApp(const FCommandDeckAction& InAction, FString InMessage, TSharedRef<FJsonObject> InPayload) override;

private:
	void Cleanup();

	void OnConnectionMessage(TSharedPtr<FJsonObject> InMessage);

	ICommandDeckConnection* Connection;

	TMap<FString, FCommandDeckAction*> Actions;
};
