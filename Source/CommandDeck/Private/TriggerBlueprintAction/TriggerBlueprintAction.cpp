// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "TriggerBlueprintAction.h"

#include "TriggerBlueprintActionManager.h"
#include "CommandDeckPlugin.h"

FCommandDeckTriggerBlueprint::FCommandDeckTriggerBlueprint(const FString& InUuid, const FString& InContext)
	: FCommandDeckAction(InUuid, InContext)
{
	RegisterEventCallback(FName("actionEvent"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckTriggerBlueprint::OnActionEvent));

	FTriggerBlueprintActionManager::Get()->OnActionCreated(*this);
}

FCommandDeckTriggerBlueprint::~FCommandDeckTriggerBlueprint()
{
	FTriggerBlueprintActionManager::Get()->OnActionDestroyed(*this);
}

void FCommandDeckTriggerBlueprint::OnActionEvent(const TSharedPtr<FJsonObject>& InPayload)
{
	FTriggerBlueprintActionManager::Get()->OnActionEvent(*this, InPayload);
}