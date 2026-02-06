// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IAssetEditorInstance;
enum class EAssetEditorCloseReason : uint8;

class FCommandDeckEditorObserver
{
public:
	static FCommandDeckEditorObserver& Get();

	void Startup();

	void Shutdown();

private:
	FCommandDeckEditorObserver();

	~FCommandDeckEditorObserver();

	FCommandDeckEditorObserver(const FCommandDeckEditorObserver&) = delete;
	FCommandDeckEditorObserver& operator=(const FCommandDeckEditorObserver&) = delete;

	void OnAssetOpenedInEditor(UObject* InAsset, IAssetEditorInstance* InEditorInstance);

	void OnAssetEditorRequestClose(UObject* InAsset, EAssetEditorCloseReason InCloseReason);
	
	FDelegateHandle AssetOpenedInEditorHandle;

	FDelegateHandle AssetEditorRequestCloseHandle;

	bool bIsInitialized;
};