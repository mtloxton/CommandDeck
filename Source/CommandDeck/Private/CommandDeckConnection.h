// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "ICommandDeckConnection.h"
#include "Runtime/Launch/Resources/Version.h"
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2)
#include "Engine/TimerHandle.h"
#else
#include "Engine/EngineTypes.h"
#endif

class IWebSocket;
class FTimerManager;

class FCommandDeckConnection : public ICommandDeckConnection
{
public:
	FCommandDeckConnection();

	virtual ~FCommandDeckConnection();

	virtual void Connect() override;

	virtual void Disconnect() override;

	virtual void Send(const FString& InMessage) override;

	virtual void Send(TSharedRef<FJsonObject> InMessage) override;

	virtual ECommandDeckConnectionStatus GetStatus() const override;

private:
	void CreateWebSocket();

	FTimerManager* GetTimerManager() const;

	FTimerHandle ConnectTimerHandle;

	TSharedPtr<IWebSocket> WebSocket;

	int RemainingAttempts;
};