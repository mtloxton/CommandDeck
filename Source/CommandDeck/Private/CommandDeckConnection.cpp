// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "CommandDeckConnection.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "IWebSocket.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"
#include "WebSocketsModule.h"

#include "CommandDeckModule.h"
#include "CommandDeckSettings.h"

namespace
{
	static constexpr float ConnectDelaySeconds = 10.0f;

	static constexpr int MaxConnectionAttempts = 5;
}

FCommandDeckConnection::FCommandDeckConnection()
{

}

FCommandDeckConnection::~FCommandDeckConnection()
{
	if (ConnectTimerHandle.IsValid())
	{
		if (FTimerManager* TimerManager = GetTimerManager())
		{
			TimerManager->ClearTimer(ConnectTimerHandle);
		}
		ConnectTimerHandle.Invalidate();
	}

	WebSocket->OnConnected().Clear();
	WebSocket->OnConnectionError().Clear();
	WebSocket->OnClosed().Clear();
	WebSocket->OnMessage().Clear();

	if (WebSocket.IsValid())
	{
		WebSocket->Close();
	}
	WebSocket.Reset();
}

void FCommandDeckConnection::Connect()
{
	UE_LOG(LogCommandDeck, Verbose, TEXT("Attempting to connect..."));

	if (!WebSocket.IsValid())
	{
		CreateWebSocket();
	}

	if (WebSocket.IsValid())
	{
		WebSocket->Connect();
	}
}


void FCommandDeckConnection::CreateWebSocket()
{
	ensure(!WebSocket.IsValid());

	RemainingAttempts = ::MaxConnectionAttempts;
	
	const UCommandDeckSettings* Settings = GetDefault<UCommandDeckSettings>();
	const FString ServerUrl = FString::Printf(TEXT("ws://%s:%d/"), *Settings->ServerAddress, Settings->ServerPort);

	TArray<FString> Protocols;
	TMap<FString, FString> UpgradeHeaders;

	FWebSocketsModule* WebSocketsModule = FModuleManager::GetModulePtr<FWebSocketsModule>(TEXT("WebSockets"));
	if (WebSocketsModule == nullptr)
	{
		WebSocketsModule = FModuleManager::LoadModulePtr<FWebSocketsModule>(TEXT("WebSockets"));
	}
	if (WebSocketsModule != nullptr)
	{
#if WITH_WEBSOCKETS
		WebSocket = WebSocketsModule->CreateWebSocket(ServerUrl, Protocols, UpgradeHeaders);
#endif
		if (WebSocket.IsValid())
		{
			WebSocket->OnConnected().AddLambda([this, ServerUrl]() -> void
				{
					UE_LOG(LogCommandDeck, Log, TEXT("Connected: url=%s"), *ServerUrl);

					if (ConnectTimerHandle.IsValid())
					{
						if (FTimerManager* TimerManager = GetTimerManager())
						{
							TimerManager->ClearTimer(ConnectTimerHandle); // Clear timer on successful connection
						}
					}

					OnConnected.Broadcast();
				});

			WebSocket->OnConnectionError().AddLambda([this, ServerUrl](const FString& Error) -> void
				{
					UE_LOG(LogCommandDeck, Verbose, TEXT("Connection attempt failed: error='%s'"), *Error);
					if (this->RemainingAttempts > 0)
					{
						--this->RemainingAttempts;
						UE_LOG(LogCommandDeck, Verbose, TEXT("Scheduling connection attempt: connectDelaySeconds=%.0f, remainingAttempts=%d"), ConnectDelaySeconds, this->RemainingAttempts);

						if (FTimerManager* TimerManager = GetTimerManager())
						{
							TimerManager->SetTimer(
								this->ConnectTimerHandle,
								FTimerDelegate::CreateRaw(this, &FCommandDeckConnection::Connect),
								::ConnectDelaySeconds,
								false);
						}
						else
						{
							UE_LOG(LogCommandDeck, Warning, TEXT("Timer Manager not available, attempting to connect immediately"));
							AsyncTask(ENamedThreads::GameThread, [this]()
								{
									this->Connect();
								});
						}
					}
					else
					{
						UE_LOG(LogCommandDeck, Warning, TEXT("Failed to connect: url=%s"), *ServerUrl);
					}
				});

			WebSocket->OnClosed().AddLambda([this, ServerUrl](int32 StatusCode, const FString& Reason, bool bWasClean) -> void
				{
					UE_LOG(LogCommandDeck, Log, TEXT("Disconnected: statusCode=%i, reason='%s', wasClean=%i"), StatusCode, *Reason, bWasClean);

					OnClosed.Broadcast();

					RemainingAttempts = ::MaxConnectionAttempts;

					UE_LOG(LogCommandDeck, Verbose, TEXT("Scheduling connection attempt: connectDelaySeconds=%.0f, remainingAttempts=%d"), ConnectDelaySeconds, this->RemainingAttempts);

					if (FTimerManager* TimerManager = GetTimerManager())
					{
						TimerManager->SetTimer(
							this->ConnectTimerHandle,
							FTimerDelegate::CreateRaw(this, &FCommandDeckConnection::Connect),
							::ConnectDelaySeconds,
							false);
					}
					else
					{
						UE_LOG(LogCommandDeck, Warning, TEXT("Timer Manager not available, attempting to connect immediately"));
						AsyncTask(ENamedThreads::GameThread, [this]()
							{
								this->Connect();
							});
					}
				});

			WebSocket->OnMessage().AddLambda([this](const FString& InMessage) -> void
				{
					FDateTime StartTime = FDateTime::UtcNow();

					TSharedPtr<FJsonObject> messageJsonObject;
					TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(InMessage);

					bool bResult = FJsonSerializer::Deserialize(jsonReader, messageJsonObject);
					ensure(bResult);

					FString Event = messageJsonObject->GetStringField(TEXT("event"));
					if (Event != TEXT("serverShutdown"))
					{
						OnMessage.Broadcast(messageJsonObject);
					}
					else
					{
						WebSocket->Close();
					}

					FTimespan Elapsed = FDateTime::UtcNow() - StartTime;
					UE_LOG(LogCommandDeck, Verbose, TEXT("Processed message: message=%s, elapsedMs=%f"), *InMessage, Elapsed.GetTotalMilliseconds());
				});
		}
		else
		{
			UE_LOG(LogCommandDeck, Error, TEXT("Failed to create WebSocket instance"));
		}
	}
	else
	{
		UE_LOG(LogCommandDeck, Error, TEXT("Failed to get the WebSockets module"));
	}
}

void FCommandDeckConnection::Disconnect()
{
	if (WebSocket.IsValid())
	{
		if (WebSocket->IsConnected())
			OnClosed.Broadcast();

		WebSocket->Close();
	}
}

void FCommandDeckConnection::Send(const FString& InMessage)
{
	if (WebSocket.IsValid() && WebSocket->IsConnected())
	{
		UE_LOG(LogCommandDeck, Verbose, TEXT("Sending message to App: message=%s"), *InMessage);
		WebSocket->Send(InMessage);
	}
	else
	{
		UE_LOG(LogCommandDeck, Warning, TEXT("Not connected! Cannot send message: message=%s"), *InMessage);
	}
}

void FCommandDeckConnection::Send(TSharedRef<FJsonObject> InMessage)
{
	FString OutputString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(InMessage, JsonWriter);

	Send(OutputString);
}

ECommandDeckConnectionStatus FCommandDeckConnection::GetStatus() const
{
	if (!WebSocket.IsValid())
		return ECommandDeckConnectionStatus::Disconnected;
	else if (!WebSocket->IsConnected())
		return ECommandDeckConnectionStatus::Pending;
	else
		return ECommandDeckConnectionStatus::Connected;
}

FTimerManager* FCommandDeckConnection::GetTimerManager() const
{
#if WITH_EDITOR
	if (GEditor)
	{
		if (GEditor->IsTimerManagerValid())
		{
			return &GEditor->GetTimerManager().Get();
		}
	}
	else
#endif
	if (GEngine)
	{
		// TODO this crashes on shutdown
		const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
		for (const FWorldContext& WorldContext : WorldContexts)
		{
			if (WorldContext.WorldType == EWorldType::Game && WorldContext.OwningGameInstance)
			{
				return &WorldContext.OwningGameInstance->GetTimerManager();
			}
		}
	}
	else
	{
		UE_LOG(LogCommandDeck, Warning, TEXT("Failed to get TimerManager instance"));
	}

	return nullptr;
}
