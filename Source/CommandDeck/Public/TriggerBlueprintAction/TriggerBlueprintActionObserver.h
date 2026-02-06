// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/NoExportTypes.h"

#include "TriggerBlueprintActionObserver.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnActionKeyDown);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnActionKeyUp, float, ElapsedMs);

DECLARE_DYNAMIC_DELEGATE(FOnActionDialDown);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnActionDialRotate, int, Ticks);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnActionDialUp, float, ElapsedMs);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnActionDialValueChanged, float, Value);

UCLASS(BlueprintType)
class COMMANDDECK_API UCommandDeckActionObserver : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Command Deck", meta = (
		WorldContext = "WorldContextObject",
		DisplayName = "Observe Key Action",
		ToolTip = "Observes key events for every Trigger Blueprint action with a matching identifier.\nGuide: https://go.commanddeck.io/blueprint"))
	static void ObserveCommandDeckKeyAction(
		UObject* WorldContextObject, FString Identifier, FOnActionKeyDown OnKeyDown, FOnActionKeyUp OnKeyUp);

	UFUNCTION(BlueprintCallable, Category = "Command Deck", meta = (
		WorldContext = "WorldContextObject",
		DisplayName = "Observe Dial Action",
		ToolTip = "Observes key events for every Trigger Blueprint action with a matching identifier.\nGuide: https://go.commanddeck.io/blueprint"))
	static void ObserveCommandDeckDialAction(
		UObject* WorldContextObject, FString Identifier,
		FOnActionDialDown OnDialDown, FOnActionDialUp OnDialUp, FOnActionDialRotate OnDialRotate);

	UFUNCTION(BlueprintCallable, Category = "Command Deck", meta = (
		WorldContext = "WorldContextObject",
		DisplayName = "Observe Dial Value Action",
		ToolTip = "Observes dial rotation events and converts it to a value within the specified range. Useful for reducing the need for additional Blueprint nodes when observing the Dial Rotate event.\nGuide: https://go.commanddeck.io/blueprint"))
	static void ObserveCommandDeckDialActionValue(
		UObject* WorldContextObject, FString Identifier,
		FOnActionDialValueChanged OnDialRotate, float StartValue = 0.0, float MinValue = 0.0, float MaxValue = 1.0, int StepCount = 10);

private:
	struct FCallbackState
	{
		double PressedTimestamp = 0.0;

		double CurrentValue = 0.0;

		double MinValue = 0.0;

		double MaxValue = 1.0;

		int StepCount = 10;
	};
};