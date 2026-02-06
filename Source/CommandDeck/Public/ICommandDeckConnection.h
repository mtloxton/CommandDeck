// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"

class FJsonObject;

DECLARE_MULTICAST_DELEGATE(CommandDeckOnConnectedDelegate);

DECLARE_MULTICAST_DELEGATE_OneParam(CommandDeckOnConnectionMessageDelegate, TSharedPtr<FJsonObject> /*Message*/);

DECLARE_MULTICAST_DELEGATE(CommandDeckOnConnectionClosedDelegate);

enum class ECommandDeckConnectionStatus : uint8
{
	Disconnected = 0,
	Pending = 1,
	Connected = 2,
};

class COMMANDDECK_API ICommandDeckConnection
{
public:
    virtual ~ICommandDeckConnection() = default;

    virtual void Connect() = 0;

    virtual void Disconnect() = 0;

    virtual void Send(const FString& InMessage) = 0;

    virtual void Send(TSharedRef<FJsonObject> InMessage) = 0;

    virtual ECommandDeckConnectionStatus GetStatus() const = 0;

    CommandDeckOnConnectedDelegate OnConnected;

    CommandDeckOnConnectionMessageDelegate OnMessage;

    CommandDeckOnConnectionClosedDelegate OnClosed;
};