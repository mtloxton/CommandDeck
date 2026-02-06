// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "UObject/NoExportTypes.h"

#include "CommandDeckAction.h"

#include "TriggerBlueprintActionBinding.generated.h"

UCLASS(BlueprintType)
class COMMANDDECK_API UTriggerBlueprintActionBinding : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Command Deck | Trigger Blueprint", meta = (
		WorldContext = "WorldContextObject",
		ToolTip = "Binds to a Trigger Blueprint action with the specified identifier, allowing you to control and update its state from Blueprint.\nGuide: https://go.commanddeck.io/blueprint"))
	static UTriggerBlueprintActionBinding* BindTriggerBlueprintAction(UObject* WorldContextObject, FString Identifier);

	UFUNCTION(BlueprintCallable, Category = "Command Deck | Trigger Blueprint", meta = (
		ToolTip = "Returns true if this binding is currently active and associated with an action.\nGuide: https://go.commanddeck.io/blueprint"))
	bool IsBound();

	UFUNCTION(BlueprintCallable, Category = "Command Deck | Trigger Blueprint", meta = (
		ToolTip = "Sets the display title for the bound Trigger Blueprint action.\nGuide: https://go.commanddeck.io/blueprint"))
	void SetTitle(FString Title);

	UFUNCTION(BlueprintCallable, Category = "Command Deck | Trigger Blueprint", meta = (
		ToolTip = "Sets the display value for the bound Trigger Blueprint action.\nGuide: https://go.commanddeck.io/blueprint"))
	void SetValue(float Value);

	UFUNCTION(BlueprintCallable, Category = "Command Deck | Trigger Blueprint | Key", meta = (
		ToolTip = "Shows an alert indicator on the bound key action, typically used to notify the user of an error or important event.\nGuide: https://go.commanddeck.io/blueprint"))
	void ShowAlert();

	UFUNCTION(BlueprintCallable, Category = "Command Deck | Trigger Blueprint", meta = (
		ToolTip = "Shows a success indicator on the bound action, typically used to acknowledge the Blueprint executed successfully.\nGuide: https://go.commanddeck.io/blueprint"))
	void ShowOk();

	virtual void BeginDestroy() override;

protected:
	friend class FTriggerBlueprintActionManager;

	void BindToAction(const FCommandDeckAction& InAction);

	void ReleaseFromAction(const FCommandDeckAction& InAction);

	void ReleaseActions();

private:
	void SendValue();

	void OnTimerElapsed();

	UObject* WorldContextObject;

	FString Identifier;

	TArray<const FCommandDeckAction*> Actions;

	float Value;

	float PendingValue;

	FTimerHandle ThrottleTimerHandle;

	float ThrottleInterval = 0.1f; // 100ms
};