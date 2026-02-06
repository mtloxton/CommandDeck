// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "UObject/NoExportTypes.h"

#include "CommandDeckAction.h"

class UTriggerBlueprintActionBinding;

enum class ECommandDeckActionEventType : uint8
{
	KeyDown,
	KeyUp,
	DialDown,
	DialUp,
	DialRotate
};

class FTriggerBlueprintActionManager
{
public:
	static FTriggerBlueprintActionManager* Get();
	
	void RegisterCallback(ECommandDeckActionEventType InEventType,
		FString InIdentifier,
		TFunction<void(const TSharedPtr<FJsonObject>, TSharedPtr<void>)> InCallback,
		TSharedPtr<void> InCallbackData);

	void RegisterBinding(UTriggerBlueprintActionBinding& InActionBinding, FString InIdentifier);

	void UnregisterBinding(FString InIdentifier);

protected:
	friend class FCommandDeckTriggerBlueprint;

	void OnActionCreated(const FCommandDeckAction& InAction);

	void OnActionDestroyed(const FCommandDeckAction& InAction);

	void OnActionEvent(const FCommandDeckAction& InAction, const TSharedPtr<FJsonObject> InPayload);

private:
	void Initialize();

	struct FCallbackWithUserData
	{
		TFunction<void(const TSharedPtr<FJsonObject>, TSharedPtr<void>)> Callback;
		TSharedPtr<void> UserData;
		FDelegateHandle Handle;

		FCallbackWithUserData(
			TFunction<void(const TSharedPtr<FJsonObject>, TSharedPtr<void>)> InCallback,
			TSharedPtr<void> InUserData,
			FDelegateHandle InHandle)
			: Callback(InCallback), UserData(InUserData), Handle(InHandle)
		{
		}
	};

	struct FCommandDeckActionEntry
	{
		FString Identifier;
		UTriggerBlueprintActionBinding* Binding;
		TMap<ECommandDeckActionEventType, TArray<FCallbackWithUserData>> Callbacks;

		FCommandDeckActionEntry() : Identifier(), Binding(nullptr) {}
		FCommandDeckActionEntry(const FString& InIdentifier, const FCommandDeckAction* InAction)
			: Identifier(InIdentifier), Binding(nullptr)
		{

		}
	};

	TMap<FString, FCommandDeckActionEntry> IdentifierToEntryMap;

	struct FAction
	{
		const FCommandDeckAction& Action;
		FString Identifier;
		bool IsPropertyInspectorVisible;

		FAction(const FCommandDeckAction& InAction, const FString& InIdentifier)
			: Action(InAction), Identifier(InIdentifier), IsPropertyInspectorVisible(false)
		{}

		FAction(const FCommandDeckAction& InAction)
			: Action(InAction), IsPropertyInspectorVisible(false)
		{}
	};

	TMap<FString, FAction> ContextToActionMap;

	FCommandDeckActionEntry* FindEntryForActionContext(const FString& InActionContext);

	void UpdatePropertyInspector();

	int ExecuteCallbacks(const FString& InIdentifier, ECommandDeckActionEventType InEventType, const TSharedPtr<FJsonObject> InPayload) const;

	const FCommandDeckAction* GetAction(FString InIdentifier);

	static void OnPostWorldInitialize(UWorld* InWorld, const UWorld::InitializationValues InInitValues);

	TArray<UWorld*> Worlds;
};