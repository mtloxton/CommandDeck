// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "CommandDeckEditorObserver.h"

#if WITH_EDITOR
#include "Subsystems/AssetEditorSubsystem.h"
#include "Toolkits/AssetEditorToolkit.h"
#endif
#include "UiCommandRegistry.h"
#include "CommandDeckModule.h"

FCommandDeckEditorObserver::FCommandDeckEditorObserver()
	: bIsInitialized(false)
{
}

FCommandDeckEditorObserver::~FCommandDeckEditorObserver()
{
	Shutdown();
}

FCommandDeckEditorObserver& FCommandDeckEditorObserver::Get()
{
	static FCommandDeckEditorObserver Instance;
	return Instance;
}

void FCommandDeckEditorObserver::Startup()
{
	if (bIsInitialized)
	{
		UE_LOG(LogCommandDeck, Warning, TEXT("CommandDeckEditorObserver::Startup() called but already initialized"));
		return;
	}
#if WITH_EDITOR
	if (GEditor)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			AssetOpenedInEditorHandle = AssetEditorSubsystem->OnAssetOpenedInEditor().AddRaw(
				this, &FCommandDeckEditorObserver::OnAssetOpenedInEditor);

			AssetEditorRequestCloseHandle = AssetEditorSubsystem->OnAssetEditorRequestClose().AddRaw(
				this, &FCommandDeckEditorObserver::OnAssetEditorRequestClose);
		}
	}
#endif
	bIsInitialized = true;
}

void FCommandDeckEditorObserver::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}
#if WITH_EDITOR
	if (GEditor)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			if (AssetOpenedInEditorHandle.IsValid())
			{
				AssetEditorSubsystem->OnAssetOpenedInEditor().Remove(AssetOpenedInEditorHandle);
			}

			if (AssetEditorRequestCloseHandle.IsValid())
			{
				AssetEditorSubsystem->OnAssetEditorRequestClose().Remove(AssetEditorRequestCloseHandle);
			}
		}
	}
#endif
	AssetOpenedInEditorHandle.Reset();

	AssetEditorRequestCloseHandle.Reset();

	bIsInitialized = false;
}

void FCommandDeckEditorObserver::OnAssetOpenedInEditor(UObject* InAsset, IAssetEditorInstance* InEditorInstance)
{
#if WITH_EDITOR
	if (!InEditorInstance || !InAsset)
	{
		return;
	}

	const FName EditorName = InEditorInstance->GetEditorName();

	UE_LOG(LogCommandDeck, Verbose, TEXT("Editor opened for asset: editorName=%s, assetName=%s"), *EditorName.ToString(), *InAsset->GetName());

	FAssetEditorToolkit* AssetEditorToolkit = static_cast<FAssetEditorToolkit*>(InEditorInstance);

	if (AssetEditorToolkit != nullptr)
	{
		TSharedPtr<FUICommandList> ToolkitCommands = AssetEditorToolkit->GetToolkitCommands();

		const FName ContextName = AssetEditorToolkit->GetToolkitContextFName();

		if (ToolkitCommands.IsValid() && !ContextName.IsNone())
		{
			FUiCommandRegistry::Get().RegisterToolkitCommandList(InEditorInstance, ContextName, ToolkitCommands.ToSharedRef());

			UE_LOG(LogCommandDeck, Verbose, TEXT("Toolkit command list registered for context: contextName=%s"), *ContextName.ToString());
		}
		else
		{
			UE_LOG(LogCommandDeck, Warning, TEXT("Toolkit has invalid command list or context name: editorName=%s"), *EditorName.ToString());
		}
	}
	else
	{
		UE_LOG(LogCommandDeck, Verbose, TEXT("Ignoring editor that does not inherit FAssetEditorToolkit: editorName=%s"), *EditorName.ToString());
	}
#endif
}

void FCommandDeckEditorObserver::OnAssetEditorRequestClose(UObject* InAsset, EAssetEditorCloseReason InCloseReason)
{
#if WITH_EDITOR
	if (!InAsset)
	{
		return;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
	{
		UE_LOG(LogCommandDeck, Warning, TEXT("Failed to get UAssetEditorSubsystem"));
		return;
	}

	IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(InAsset, false);
	if (EditorInstance)
	{
		FUiCommandRegistry::Get().UnregisterToolkitCommandList(EditorInstance);
	}
	else
	{
		UE_LOG(LogCommandDeck, Warning, TEXT("Failed to find editor instance for asset: assetName=%s"), *InAsset->GetName());
	}
#endif
}
