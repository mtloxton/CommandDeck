// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FCommandDeckAction;
class FCommandDeckDevice;

class COMMANDDECK_API ICommandDeckPlugin
{
public:
    virtual ~ICommandDeckPlugin() = default;

	virtual void SendToApp(const FCommandDeckAction& InAction, FString InMessage) = 0;

    virtual void SendToApp(const FCommandDeckAction& InAction, FString InMessage, TSharedRef<FJsonObject> InPayload) = 0;
};