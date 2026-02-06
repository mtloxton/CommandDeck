// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "CommandDeckPlugin.h"

#include "Dom/JsonObject.h"
#include "Interfaces/IPluginManager.h"
#include "JsonObjectConverter.h"
#include "Misc/EngineVersion.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "CommandDeckActionFactory.h"
#include "CommandDeckConnection.h"
#include "CommandDeckModule.h"

FCommandDeckPlugin::FCommandDeckPlugin(ICommandDeckConnection* InConnection)
{
	Connection = InConnection;

	Connection->OnConnected.AddLambda([this]()
	{
		const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("CommandDeck"));

		TSharedRef<FJsonObject> commandJsonObject = MakeShared<FJsonObject>();
		commandJsonObject->SetStringField("event", "hello");
		commandJsonObject->SetStringField("pluginVersion", plugin->GetDescriptor().VersionName);
		commandJsonObject->SetStringField("engineVersion", FEngineVersion::Current().ToString());

		Connection->Send(commandJsonObject);
	});

	Connection->OnMessage.AddLambda([this](TSharedPtr<FJsonObject> InMessage)
		{
			OnConnectionMessage(InMessage);
		});

	Connection->OnClosed.AddLambda([this]()
		{
			Cleanup();
		});
}

FCommandDeckPlugin::~FCommandDeckPlugin()
{
	Cleanup();
}

void FCommandDeckPlugin::Cleanup()
{
	for (auto& actionKvp : Actions)
	{
		UE_LOG(LogCommandDeck, Verbose, TEXT("Destroyed action: action=%s, context=%s"),*actionKvp.Value->GetUuid(), *actionKvp.Value->GetContext());

		delete actionKvp.Value;
	}

	Actions.Empty();
}

void FCommandDeckPlugin::OnConnectionMessage(TSharedPtr<FJsonObject> InMessage)
{
	const FName Event = FName(*InMessage->GetStringField(TEXT("event")));
	ensure(!Event.IsNone());

	const FString Context = InMessage->GetStringField(TEXT("context"));
	ensure(!Context.IsEmpty());

	if (Event == FName("onCreate"))
	{
		const FString Uuid = InMessage->GetStringField(TEXT("action"));
		ensure(!Uuid.IsEmpty());

		if (Actions.Contains(Context))
		{
			UE_LOG(LogCommandDeck, Warning, TEXT("Action with context already exists: action=%s, context=%s"), *Uuid, *Context);

			auto actionInstance = Actions[Context];
			Actions.Remove(Context);
			delete actionInstance;
		}

		FCommandDeckAction* actionInstance = FCommandDeckActionFactory::Get().Create(Uuid, Context);

		UE_LOG(LogCommandDeck, Verbose, TEXT("Created action: action=%s, context=%s"), *Uuid, *Context);

		Actions.Add(Context, actionInstance);
	}
	else if (Event == FName("onDestroy"))
	{
		ensure(Actions.Contains(Context));

		auto actionInstance = Actions[Context];

		Actions.Remove(Context);

		UE_LOG(LogCommandDeck, Verbose, TEXT("Destroyed action: action=%s, context=%s"), *actionInstance->GetUuid(), *actionInstance->GetContext());

		delete actionInstance;
	}
	else
	{
		TSharedPtr<FJsonObject> Payload = InMessage->GetObjectField(TEXT("payload"));

		if (Actions.Contains(Context))
		{
			auto commandDeckAction = Actions[Context];
			if (!commandDeckAction->OnEvent(Event, Payload))
			{
				UE_LOG(LogCommandDeck, Warning, TEXT("Unhandled event: event=%s, context=%s"), *Event.ToString(), *Context);
			}
		}
		else
		{
			UE_LOG(LogCommandDeck, Warning, TEXT("No action to handle event: event=%s, context=%s"), *Event.ToString(), *Context);
		}
	}
}

void FCommandDeckPlugin::SendToApp(const FCommandDeckAction& InAction, FString InMessage)
{
	ensure(!InMessage.IsEmpty() && InMessage != TEXT("hello")); // Reserved for internal use only

	TSharedRef<FJsonObject> commandJsonObject = MakeShared<FJsonObject>();
	commandJsonObject->SetStringField("context", InAction.GetContext());
	commandJsonObject->SetStringField("event", InMessage);

	Connection->Send(commandJsonObject);
}

void FCommandDeckPlugin::SendToApp(const FCommandDeckAction& InAction, FString InMessage, TSharedRef<FJsonObject> InPayload)
{
	ensure(!InMessage.IsEmpty() && InMessage != TEXT("hello")); // Reserved for internal use only

	TSharedRef<FJsonObject> commandJsonObject = MakeShared<FJsonObject>();
	commandJsonObject->SetStringField("context", InAction.GetContext());
	commandJsonObject->SetStringField("event", InMessage);
	commandJsonObject->SetObjectField("payload", InPayload);

	Connection->Send(commandJsonObject);
}