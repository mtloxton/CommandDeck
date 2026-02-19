// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "TriggerBlueprintActionManager.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "CommandDeckAction.h"
#include "CommandDeckPlugin.h"
#include "TriggerBlueprintAction.h"
#include "TriggerBlueprintAction/TriggerBlueprintActionObserver.h"
#include "TriggerBlueprintAction/TriggerBlueprintActionBinding.h"

FTriggerBlueprintActionManager* FTriggerBlueprintActionManager::Get()
{
	static FTriggerBlueprintActionManager* k_SingletonInstance = nullptr;

	if (!k_SingletonInstance && FCommandDeckModule::GetPlugin() != nullptr)
	{
		k_SingletonInstance = new FTriggerBlueprintActionManager();
		k_SingletonInstance->Initialize();
	}

	return k_SingletonInstance;
}

void FTriggerBlueprintActionManager::OnPostWorldInitialize(UWorld* InWorld, const UWorld::InitializationValues InInitValues)
{
	if (InWorld->IsGameWorld())
	{
		FTriggerBlueprintActionManager::Get()->Worlds.Add(InWorld);
		FTriggerBlueprintActionManager::Get()->UpdatePropertyInspector();
	}
}

void FTriggerBlueprintActionManager::Initialize()
{
	Worlds.Empty();

	FWorldDelegates::OnPostWorldInitialization.AddStatic(&FTriggerBlueprintActionManager::OnPostWorldInitialize);

	FWorldDelegates::OnPostWorldCleanup.AddLambda([this](UWorld* InWorld, bool InSessionEnded, bool bInCleanupResources)
	{
		if (Worlds.Contains(InWorld))
		{
			Worlds.Remove(InWorld);

			if (Worlds.Num() == 0)
			{
				for (TPair<FString, FCommandDeckActionEntry>& actionEntryPair : IdentifierToEntryMap)
				{
					FCommandDeckActionEntry& actionEntry = actionEntryPair.Value;
					if (actionEntry.Binding != nullptr)
					{
						actionEntry.Binding->ReleaseActions();
						actionEntry.Binding = nullptr;
					}

					for (TPair<ECommandDeckActionEventType, TArray<FCallbackWithUserData>> callbackPair : actionEntry.Callbacks)
					{
						for (FCallbackWithUserData& callbackWithUserData : callbackPair.Value)
						{
							callbackWithUserData.Handle.Reset();
						}
						callbackPair.Value.Empty();
					}
					actionEntry.Callbacks.Empty();
				}
				IdentifierToEntryMap.Empty();

				UpdatePropertyInspector();
			}
		}
	});
}

void FTriggerBlueprintActionManager::RegisterCallback(ECommandDeckActionEventType InEventType, FString InIdentifier, TFunction<void(const TSharedPtr<FJsonObject>, TSharedPtr<void>)> InCallback, TSharedPtr<void> InCallbackData)
{
	ensure(!InIdentifier.IsEmpty());

	FCommandDeckActionEntry& entry = IdentifierToEntryMap.FindOrAdd(InIdentifier);

	FDelegateHandle handle(FDelegateHandle::EGenerateNewHandleType::GenerateNewHandle);

	FCallbackWithUserData callbackWithUserData(InCallback, InCallbackData, handle);

	entry.Callbacks.FindOrAdd(InEventType).Add(callbackWithUserData);

	UpdatePropertyInspector();
}

void FTriggerBlueprintActionManager::RegisterBinding(UTriggerBlueprintActionBinding& InActionBinding, FString InIdentifier)
{
	FCommandDeckActionEntry& entry = IdentifierToEntryMap.FindOrAdd(InIdentifier);
	entry.Identifier = InIdentifier;
	entry.Binding = &InActionBinding;

	for (auto kvp : ContextToActionMap)
	{
		if (kvp.Value.Identifier.Equals(InIdentifier))
		{
			InActionBinding.BindToAction(kvp.Value.Action);
		}
	}

	UpdatePropertyInspector();
}

void FTriggerBlueprintActionManager::UnregisterBinding(FString InIdentifier)
{
	FCommandDeckActionEntry* entry = IdentifierToEntryMap.Find(InIdentifier);
	if (entry != nullptr)
	{
		entry->Binding = nullptr;
	}

	UpdatePropertyInspector();
}

const FCommandDeckAction* FTriggerBlueprintActionManager::GetAction(FString InIdentifier)
{
	for (TPair<FString, FAction>& Pair : ContextToActionMap)
	{
		if (Pair.Value.Identifier.Equals(InIdentifier))
		{
			return &Pair.Value.Action;
		}
	}

	return nullptr;
}

void FTriggerBlueprintActionManager::OnActionCreated(const FCommandDeckAction& InAction)
{
	ContextToActionMap.Add(InAction.GetContext(), FAction(InAction));
}

void FTriggerBlueprintActionManager::OnActionDestroyed(const FCommandDeckAction& InAction)
{
	auto contextEntry = ContextToActionMap.Find(InAction.GetContext());

	FCommandDeckActionEntry* entry = IdentifierToEntryMap.Find(contextEntry->Identifier);
	if (entry != nullptr && entry->Binding != nullptr)
	{
		entry->Binding->ReleaseFromAction(InAction);
	}

	ContextToActionMap.Remove(InAction.GetContext());
}

void FTriggerBlueprintActionManager::OnActionEvent(const FCommandDeckAction& InAction, const TSharedPtr<FJsonObject> InPayload)
{
	FString Identifier = ContextToActionMap.Find(InAction.GetContext())->Identifier;
	int ExecutionCount = -1;

	FString EventType = InPayload->GetStringField(TEXT("type"));

	if (EventType.Equals(TEXT("onDialDown")))
	{
		ExecutionCount = ExecuteCallbacks(Identifier, ECommandDeckActionEventType::DialDown, InPayload);
	}
	else if (EventType.Equals(TEXT("onDialRotate")))
	{
		ExecutionCount = ExecuteCallbacks(Identifier, ECommandDeckActionEventType::DialRotate, InPayload);
	}
	else if (EventType.Equals(TEXT("onDialUp")))
	{
		ExecutionCount = ExecuteCallbacks(Identifier, ECommandDeckActionEventType::DialUp, InPayload);
	}
	else if (EventType.Equals(TEXT("onKeyDown")))
	{
		ExecutionCount = ExecuteCallbacks(Identifier, ECommandDeckActionEventType::KeyDown, InPayload);
	}
	else if (EventType.Equals(TEXT("onKeyUp")))
	{
		ExecutionCount = ExecuteCallbacks(Identifier, ECommandDeckActionEventType::KeyUp, InPayload);
	}
	else if (EventType.Equals(TEXT("onPropertyInspectorDidAppear")))
	{
		ContextToActionMap.Find(InAction.GetContext())->IsPropertyInspectorVisible = true;
		UpdatePropertyInspector();
	}
	else if (EventType.Equals(TEXT("onPropertyInspectorDidDisappear")))
	{
		ContextToActionMap.Find(InAction.GetContext())->IsPropertyInspectorVisible = false;
	}
	else if (EventType.Equals(TEXT("onSettingUpdate")))
	{
		FString newIdentifier = InPayload->GetStringField(TEXT("identifier"));

		// Check if the identifier has changed
		FAction* actionContext = ContextToActionMap.Find(InAction.GetContext());

		if (newIdentifier != actionContext->Identifier)
		{
			// Disassociate the action from the old identifier
			if (!actionContext->Identifier.IsEmpty())
			{
				FCommandDeckActionEntry* actionEntry = IdentifierToEntryMap.Find(actionContext->Identifier);
				if (actionEntry != nullptr && actionEntry->Binding != nullptr)
				{
					actionEntry->Binding->ReleaseFromAction(InAction);
				}
			}

			// Update the identifier associated with this action
			actionContext->Identifier = newIdentifier;

			// Associate the action with the new identifier
			if (!actionContext->Identifier.IsEmpty())
			{
				FCommandDeckActionEntry* actionEntry = IdentifierToEntryMap.Find(actionContext->Identifier);
				if (actionEntry != nullptr && actionEntry->Binding != nullptr)
				{
					actionEntry->Binding->BindToAction(InAction);
				}
			}
		}
	}

	if (ExecutionCount != -1)
	{
		TSharedRef<FJsonObject> sendToAppPayload = MakeShared<FJsonObject>();
		sendToAppPayload->SetBoolField("success", ExecutionCount != 0);
		FCommandDeckModule::GetPlugin()->SendToApp(InAction, "triggerResult", sendToAppPayload);
	}
}

FTriggerBlueprintActionManager::FCommandDeckActionEntry* FTriggerBlueprintActionManager::FindEntryForActionContext(const FString& InActionContext)
{
	// Resolve the identifier associated with this action
	auto entry = ContextToActionMap.Find(InActionContext);
	if (entry != nullptr)
	{
		return IdentifierToEntryMap.Find(entry->Identifier); // Notify all observers associated with this identifier
	}
	return nullptr;
}

void FTriggerBlueprintActionManager::UpdatePropertyInspector()
{
	// Notify any actions of the new binding if their property inspector is visible
	for (TPair<FString, FAction>& Pair : ContextToActionMap)
	{
		TArray<TSharedPtr<FJsonValue>> identifierJsonArray;

		if (!Pair.Value.IsPropertyInspectorVisible)
			continue;

		for (TPair<FString, FCommandDeckActionEntry>& actionEntry : IdentifierToEntryMap)
		{
			TSharedRef<FJsonObject> callbacksJson = MakeShared<FJsonObject>();

			auto* keyDownCallbacks = actionEntry.Value.Callbacks.Find(ECommandDeckActionEventType::KeyDown);
			callbacksJson->SetNumberField("keyDown", keyDownCallbacks ? keyDownCallbacks->Num() : 0);

			auto* keyUpCallbacks = actionEntry.Value.Callbacks.Find(ECommandDeckActionEventType::KeyUp);
			callbacksJson->SetNumberField("keyUp", keyUpCallbacks ? keyUpCallbacks->Num() : 0);

			auto* dialDownCallbacks = actionEntry.Value.Callbacks.Find(ECommandDeckActionEventType::DialDown);
			callbacksJson->SetNumberField("dialDown", dialDownCallbacks ? dialDownCallbacks->Num() : 0);

			auto* dialRotateCallbacks = actionEntry.Value.Callbacks.Find(ECommandDeckActionEventType::DialRotate);
			callbacksJson->SetNumberField("dialRotate", dialRotateCallbacks ? dialRotateCallbacks->Num() : 0);

			auto* dialUpCallbacks = actionEntry.Value.Callbacks.Find(ECommandDeckActionEventType::DialUp);
			callbacksJson->SetNumberField("dialUp", dialUpCallbacks ? dialUpCallbacks->Num() : 0);

			TSharedRef<FJsonObject> identifierJson = MakeShared<FJsonObject>();

			identifierJson->SetStringField("identifier", actionEntry.Key);
			identifierJson->SetObjectField("callbacks", callbacksJson);
			identifierJson->SetStringField("binding", (actionEntry.Value.Binding != nullptr) ? "true" : "false");

			identifierJsonArray.Add(MakeShared<FJsonValueObject>(identifierJson));
		}

		TSharedRef<FJsonObject> sendToAppPayload = MakeShared<FJsonObject>();
		sendToAppPayload->SetArrayField("identifiers", identifierJsonArray);
		sendToAppPayload->SetBoolField("isWorldActive", Worlds.Num() > 0 ? true : false);

		FCommandDeckModule::GetPlugin()->SendToApp(Pair.Value.Action, "worldInfo", sendToAppPayload);
	}
}

int FTriggerBlueprintActionManager::ExecuteCallbacks(const FString& InIdentifier,
	ECommandDeckActionEventType InEventType,
	const TSharedPtr<FJsonObject> InPayload) const
{
	int executionCount = 0;

	auto* entry = IdentifierToEntryMap.Find(InIdentifier);
	if (entry != nullptr)
	{
		auto* callbackEntries = entry->Callbacks.Find(InEventType);

		if (callbackEntries != nullptr)
		{
			for (const FCallbackWithUserData& Entry : *callbackEntries)
			{
				Entry.Callback(InPayload, Entry.UserData);
				executionCount++;
			}
		}
	}

	return executionCount;
}