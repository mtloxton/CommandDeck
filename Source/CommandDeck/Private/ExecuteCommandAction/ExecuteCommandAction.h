// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CommandDeckAction.h"
#include "CommandDeckActionFactory.h"

class FCommandDeckExecuteCommand : public FCommandDeckAction
{
public:
	FCommandDeckExecuteCommand(const FString& InUuid, const FString& InContext);

	virtual ~FCommandDeckExecuteCommand() override;

private:
	void OnExecute(const TSharedPtr<FJsonObject>& InPayload);

	void OnExportCommandData(const TSharedPtr<FJsonObject>& InPayload);

	FString CachedValue;

	FDelegateHandle BeginPIEDelegateHandle;
};

UE_REGISTER_COMMAND_DECK_ACTION("io.mudall.command-deck.unreal.execute-command", FCommandDeckExecuteCommand)