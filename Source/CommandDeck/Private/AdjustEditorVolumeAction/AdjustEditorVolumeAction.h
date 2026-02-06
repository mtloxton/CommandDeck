// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CommandDeckAction.h"
#include "CommandDeckActionFactory.h"

class ULevelEditorMiscSettings;

class FCommandDeckAdjustEditorVolume : public FCommandDeckAction
{
public:
	FCommandDeckAdjustEditorVolume(const FString& InUuid, const FString& InContext);

	virtual ~FCommandDeckAdjustEditorVolume() override;

private:
	void OnSetVolume(const TSharedPtr<FJsonObject>& InPayload);

	void OnAdjustVolume(const TSharedPtr<FJsonObject>& InPayload);

	void OnSetMute(const TSharedPtr<FJsonObject>& InPayload);
	
	void UpdateFromSettings(const ULevelEditorMiscSettings* InLevelEditorMiscSettings) const;

	FDelegateHandle PropertyChangedHandle;
};

UE_REGISTER_COMMAND_DECK_ACTION("io.mudall.command-deck.unreal.adjust-editor-volume", FCommandDeckAdjustEditorVolume)