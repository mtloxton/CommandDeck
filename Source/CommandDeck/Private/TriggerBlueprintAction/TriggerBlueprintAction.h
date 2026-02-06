// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CommandDeckAction.h"
#include "CommandDeckActionFactory.h"

class FCommandDeckTriggerBlueprint : public FCommandDeckAction
{
public:
	FCommandDeckTriggerBlueprint(const FString& InUuid, const FString& InContext);

	virtual ~FCommandDeckTriggerBlueprint() override;

private:
	void OnActionEvent(const TSharedPtr<FJsonObject>& InPayload);
};

UE_REGISTER_COMMAND_DECK_ACTION("io.mudall.command-deck.unreal.trigger-blueprint", FCommandDeckTriggerBlueprint)