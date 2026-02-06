// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "TriggerBlueprintAction/TriggerBlueprintActionObserver.h"

#include "Async/Async.h"

#include "CommandDeckAction.h"
#include "TriggerBlueprintActionManager.h"

void UCommandDeckActionObserver::ObserveCommandDeckKeyAction(
	UObject* InWorldContextObjet, FString InIdentifier, FOnActionKeyDown InOnKeyDown, FOnActionKeyUp InOnKeyUp)
{
	auto actionObserver = NewObject<UCommandDeckActionObserver>(InWorldContextObjet);

	FTriggerBlueprintActionManager* actionManager = FTriggerBlueprintActionManager::Get();

	if (actionManager != nullptr && !InIdentifier.IsEmpty())
	{
		TSharedPtr<FCallbackState> callbackState = MakeShared<FCallbackState>();

		if (InOnKeyDown.IsBound())
		{
			actionManager->RegisterCallback(
				ECommandDeckActionEventType::KeyDown,
				InIdentifier,
				[actionObserver, InOnKeyDown](const TSharedPtr<FJsonObject> InPayload, TSharedPtr<void> InCallbackData)
				{
					TSharedPtr<FCallbackState> callbackState = StaticCastSharedPtr<FCallbackState, void>(InCallbackData);
					callbackState->PressedTimestamp = FPlatformTime::Seconds();

					AsyncTask(ENamedThreads::GameThread, [InOnKeyDown]()
						{
							InOnKeyDown.ExecuteIfBound();
						});
				},
				callbackState
			);
		}

		if (InOnKeyUp.IsBound())
		{
			actionManager->RegisterCallback(
				ECommandDeckActionEventType::KeyUp,
				InIdentifier,
				[InOnKeyUp](const TSharedPtr<FJsonObject> InPayload, TSharedPtr<void> InCallbackData)
				{
					TSharedPtr<FCallbackState> callbackState = StaticCastSharedPtr<FCallbackState, void>(InCallbackData);
					double elapsedMs = FPlatformTime::Seconds() - callbackState->PressedTimestamp;

					AsyncTask(ENamedThreads::GameThread, [InOnKeyUp, elapsedMs]()
						{
							InOnKeyUp.ExecuteIfBound(elapsedMs);
						});
				},
				callbackState
			);
		}
	}
}

void UCommandDeckActionObserver::ObserveCommandDeckDialAction(
	UObject* InWorldContextObject, FString InIdentifier,
	FOnActionDialDown InOnDialDown, FOnActionDialUp InOnDialUp, FOnActionDialRotate InOnDialRotate)
{
	auto actionObserver = NewObject<UCommandDeckActionObserver>(InWorldContextObject);

	FTriggerBlueprintActionManager* actionManager = FTriggerBlueprintActionManager::Get();

	if (actionManager != nullptr && !InIdentifier.IsEmpty())
	{
		TSharedPtr<FCallbackState> callbackState = MakeShared<FCallbackState>();

		if (InOnDialDown.IsBound())
		{
			actionManager->RegisterCallback(
				ECommandDeckActionEventType::DialDown,
				InIdentifier,
				[InOnDialDown](const TSharedPtr<FJsonObject> InPayload, TSharedPtr<void> InCallbackData)
				{
					TSharedPtr<FCallbackState> localCallbackState = StaticCastSharedPtr<FCallbackState, void>(InCallbackData);
					localCallbackState->PressedTimestamp = FPlatformTime::Seconds();

					AsyncTask(ENamedThreads::GameThread, [InOnDialDown]()
						{
							InOnDialDown.ExecuteIfBound();
						});
				},
				MakeShared<FCallbackState>()
			);
		}

		if (InOnDialUp.IsBound())
		{
			actionManager->RegisterCallback(
				ECommandDeckActionEventType::DialUp,
				InIdentifier,
				[InOnDialUp](const TSharedPtr<FJsonObject> InPayload, TSharedPtr<void> InCallbackData)
				{
					TSharedPtr<FCallbackState> localCallbackState = StaticCastSharedPtr<FCallbackState, void>(InCallbackData);
					double elapsedMs = FPlatformTime::Seconds() - localCallbackState->PressedTimestamp;

					AsyncTask(ENamedThreads::GameThread, [InOnDialUp, elapsedMs]()
						{
							InOnDialUp.ExecuteIfBound(elapsedMs);
						});
				},
				callbackState
			);
		}

		if (InOnDialRotate.IsBound())
		{
			actionManager->RegisterCallback(
				ECommandDeckActionEventType::DialRotate,
				InIdentifier,
				[actionObserver, InOnDialRotate](const TSharedPtr<FJsonObject> InPayload, TSharedPtr<void> InCallbackData)
				{
					int rotateTicks = InPayload->GetNumberField(TEXT("ticks"));

					AsyncTask(ENamedThreads::GameThread, [InOnDialRotate, rotateTicks]()
						{
							InOnDialRotate.ExecuteIfBound(rotateTicks);
						});
				},
				MakeShared<FCallbackState>()
			);
		}
	}
}

void UCommandDeckActionObserver::ObserveCommandDeckDialActionValue(
	UObject* InWorldContextObject, FString InIdentifier,
	FOnActionDialValueChanged InOnDialValueChanged, float InStartValue, float InMinValue, float InMaxValue, int InStepCount)
{
	auto actionObserver = NewObject<UCommandDeckActionObserver>(InWorldContextObject);

	FTriggerBlueprintActionManager* actionManager = FTriggerBlueprintActionManager::Get();

	if (actionManager != nullptr && !InIdentifier.IsEmpty())
	{
		TSharedPtr<FCallbackState> callbackState = MakeShared<FCallbackState>();
		callbackState->CurrentValue = FMath::Clamp(InStartValue, InMinValue, InMaxValue);
		callbackState->MinValue = FMath::Min(InMinValue, InMaxValue);
		callbackState->MaxValue = FMath::Max(InMinValue, InMaxValue);
		callbackState->StepCount = FMath::Max(InStepCount, 1);

		if (InOnDialValueChanged.IsBound())
		{
			actionManager->RegisterCallback(
				ECommandDeckActionEventType::DialRotate,
				InIdentifier,
				[InOnDialValueChanged](const TSharedPtr<FJsonObject> InPayload, TSharedPtr<void> InCallbackData)
				{
					TSharedPtr<FCallbackState> localCallbackState = StaticCastSharedPtr<FCallbackState, void>(InCallbackData);

					int rotateTicks = InPayload->GetNumberField(TEXT("ticks"));

					// Update the current value based on the number of ticks, scaled according to the min/max range and step count
					float range = localCallbackState->MaxValue - localCallbackState->MinValue;
					float stepSize = range / localCallbackState->StepCount;
					float newValue = FMath::Clamp(localCallbackState->CurrentValue + (rotateTicks * stepSize), localCallbackState->MinValue, localCallbackState->MaxValue);

					if (localCallbackState->CurrentValue != newValue)
					{
						localCallbackState->CurrentValue = newValue;
						AsyncTask(ENamedThreads::GameThread, [InOnDialValueChanged, newValue]()
							{
								InOnDialValueChanged.ExecuteIfBound(newValue);
							});
					}
				},
				callbackState
			);
		}
	}
}