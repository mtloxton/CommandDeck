// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CommandDeckAction.h"
#include "CommandDeckActionFactory.h"

class FCommandDeckExecuteUiCommand : public FCommandDeckAction
{
public:
	FCommandDeckExecuteUiCommand(const FString& InUuid, const FString& InContext);

	virtual ~FCommandDeckExecuteUiCommand() override;

private:
	void OnCommandRegisteryUpdated();

	void OnExecute(const TSharedPtr<FJsonObject>& InPayload);

	void OnExportCommandList(const TSharedPtr<FJsonObject>& InPayload);

	void OnExportCommandInfo(const TSharedPtr<FJsonObject>& InPayload);

	FDelegateHandle CommandRegisteryUpdatedHandle;
};

UE_REGISTER_COMMAND_DECK_ACTION("io.mudall.command-deck.unreal.execute-ui-command", FCommandDeckExecuteUiCommand)