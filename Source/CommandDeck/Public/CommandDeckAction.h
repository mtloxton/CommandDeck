// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "Dom/JsonObject.h"

#include "CommandDeckModule.h"

class FCommandDeckDevice;
class FCommandDeckPlugin;

DECLARE_DELEGATE_OneParam(FCommandDeckEventCallback, const TSharedPtr<FJsonObject>& /*Payload*/);

class COMMANDDECK_API FCommandDeckAction
{
public:
	FCommandDeckAction(const FString& InUuid, const FString& InContext)
		: Uuid(InUuid), Context(InContext)
	{
	}

	virtual ~FCommandDeckAction()
	{
	}

	bool OnEvent(const FName InEvent, const TSharedPtr<FJsonObject>& InPayload)
	{
		if (const FCommandDeckEventCallback* Callback = EventCallbacks.Find(InEvent))
		{
			return Callback->ExecuteIfBound(InPayload);
		}
		else
		{
			return false;
		}
	}

	FString GetUuid() const { return Uuid; }

	FString GetContext() const { return Context; }

protected:
	void RegisterEventCallback(const FName& InEvent, const FCommandDeckEventCallback& InCallback)
	{
		EventCallbacks.Add(InEvent, InCallback);
	}

	void RegisterEventCallback(const FName& InEvent, TFunction<void(const TSharedPtr<FJsonObject>&)> InFunction)
	{
		EventCallbacks.Add(InEvent, FCommandDeckEventCallback::CreateLambda(InFunction));
	}

	void UnregisterEventCallback(const FName& InEvent)
	{
		EventCallbacks.Remove(InEvent);
	}

private:
	TMap<FName, FCommandDeckEventCallback> EventCallbacks;

	FString Uuid;

	FString Context;
};