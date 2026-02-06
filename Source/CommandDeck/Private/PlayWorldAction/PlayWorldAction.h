// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CommandDeckAction.h"
#include "CommandDeckActionFactory.h"

class FCommandDeckPlayWorld : public FCommandDeckAction
{
public:
	FCommandDeckPlayWorld(const FString& InUuid, const FString& InContext);

	virtual ~FCommandDeckPlayWorld() override;

private:
	void OnPlayInEditor(const TSharedPtr<FJsonObject>& InPayload);

	void OnEndPlay(const TSharedPtr<FJsonObject>& InPayload);

	FDelegateHandle BeginPIEDelegateHandle;

	FDelegateHandle EndPIEDelegateHandle;

	void OnBeginPIE(bool bIsSimulating);

	void OnEndPIE(bool bIsSimulating);
};

UE_REGISTER_COMMAND_DECK_ACTION("io.mudall.command-deck.unreal.play-world", FCommandDeckPlayWorld)