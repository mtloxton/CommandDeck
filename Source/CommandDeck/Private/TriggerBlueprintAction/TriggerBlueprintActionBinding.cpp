// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "TriggerBlueprintAction/TriggerBlueprintActionBinding.h"

#include "Runtime/Launch/Resources/Version.h"
#include "CommandDeckAction.h"
#include "CommandDeckPlugin.h"
#include "TriggerBlueprintActionManager.h"

UTriggerBlueprintActionBinding* UTriggerBlueprintActionBinding::BindTriggerBlueprintAction(UObject* InWorldContextObject, FString InIdentifier)
{
	auto actionBinding = NewObject<UTriggerBlueprintActionBinding>(InWorldContextObject);
	actionBinding->WorldContextObject = InWorldContextObject;
	actionBinding->Identifier = InIdentifier;
	actionBinding->Value = 0.0;
	actionBinding->PendingValue = 0.0;

	FTriggerBlueprintActionManager* actionManager = FTriggerBlueprintActionManager::Get();
	if (actionManager != nullptr)
		actionManager->RegisterBinding(*actionBinding, InIdentifier);

	return actionBinding;
}

bool UTriggerBlueprintActionBinding::IsBound()
{
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 6)
	return !Actions.IsEmpty();
#else
	return Actions.Num() > 0;
#endif
}

void UTriggerBlueprintActionBinding::SetValue(float InValue)
{
	if (InValue != PendingValue)
	{
		PendingValue = InValue;

		FTimerManager& timerManager = GetWorld()->GetTimerManager();
		if (!timerManager.IsTimerActive(ThrottleTimerHandle))
		{
			Value = PendingValue;
			SendValue();
		}
	}
}

void UTriggerBlueprintActionBinding::SetTitle(FString InTitle)
{
	for (const FCommandDeckAction* action : Actions)
	{
		TSharedRef<FJsonObject> sendToAppPayload = MakeShared<FJsonObject>();
		sendToAppPayload->SetStringField("title", InTitle);
		FCommandDeckModule::GetPlugin()->SendToApp(*action, "setTitle", sendToAppPayload);
	}

}

void UTriggerBlueprintActionBinding::ShowAlert()
{
	for (const FCommandDeckAction* action : Actions)
	{
		FCommandDeckModule::GetPlugin()->SendToApp(*action, "showAlert");
	}
}

void UTriggerBlueprintActionBinding::ShowOk()
{
	for (const FCommandDeckAction* action : Actions)
	{
		FCommandDeckModule::GetPlugin()->SendToApp(*action, "showOk");
	}
}

void UTriggerBlueprintActionBinding::BeginDestroy()
{
	FTriggerBlueprintActionManager* actionManager = FTriggerBlueprintActionManager::Get();
	if (actionManager != nullptr)
		actionManager->UnregisterBinding(Identifier);

	if (GetWorld())
	{
		FTimerManager& timerManager = GetWorld()->GetTimerManager();
		timerManager.ClearTimer(ThrottleTimerHandle);
	}

	Super::BeginDestroy();
}

void UTriggerBlueprintActionBinding::BindToAction(const FCommandDeckAction& InAction)
{
	Actions.Add(&InAction);

	SendValue();
}

void UTriggerBlueprintActionBinding::ReleaseFromAction(const FCommandDeckAction& InAction)
{
	Actions.Remove(&InAction);
}

void UTriggerBlueprintActionBinding::ReleaseActions()
{
	Actions.Empty();
}

void UTriggerBlueprintActionBinding::SendValue()
{
	for (const FCommandDeckAction* action : Actions)
	{
		TSharedRef<FJsonObject> sendToAppPayload = MakeShared<FJsonObject>();
		sendToAppPayload->SetNumberField("value", Value);
		FCommandDeckModule::GetPlugin()->SendToApp(*action, "setValue", sendToAppPayload);
	}
	FTimerManager& timerManager = GetWorld()->GetTimerManager();
	timerManager.SetTimer(ThrottleTimerHandle, this, &UTriggerBlueprintActionBinding::OnTimerElapsed, ThrottleInterval, false);
}

void UTriggerBlueprintActionBinding::OnTimerElapsed()
{
	if (PendingValue != Value)
	{
		Value = PendingValue;
		SendValue();
	}
}