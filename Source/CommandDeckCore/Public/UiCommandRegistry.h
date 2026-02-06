// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Commands/InputBindingManager.h"

DECLARE_MULTICAST_DELEGATE(FOnCommandRegistryUpdatedDelegate)

class COMMANDDECKCORE_API FUiCommandRegistry
{
public:
	static FUiCommandRegistry& Get();

	void Startup();

	void Shutdown();

	void GetCommands(TArray<TSharedPtr<FUICommandInfo>>& OutCommandInfos) const;

	TSharedPtr<FUICommandInfo> GetCommandListsForCommand(const FName InContextName, const FName InCommandName, TArray<TSharedPtr<FUICommandList>>& OutCommandLists) const;

	void RegisterToolkitCommandList(const void* InToolkitInstance, const FName InContextName, TSharedRef<FUICommandList> InCommandList);

	void UnregisterToolkitCommandList(const void* InToolkitInstance);

	void RegisterCommandList(const FName InContextName, TSharedRef<FUICommandList> InCommandList);

	void UnregisterCommandList(const FName InContextName, TSharedRef<FUICommandList> InCommandList);

	FOnCommandRegistryUpdatedDelegate OnCommandRegistryUpdated;

private:
	FUiCommandRegistry();

	FUiCommandRegistry(const FUiCommandRegistry&) = delete;
	FUiCommandRegistry& operator=(const FUiCommandRegistry&) = delete;

	struct FCommandListInfo
	{
		const FName ContextName;
		const FName CommandName;
		TSharedPtr<FUICommandInfo> CommandInfo;
		TArray<TSharedRef<FUICommandList>> CommandLists;

		FCommandListInfo(const FName InContextName, const FName InCommandName, TSharedPtr<FUICommandInfo> InCommandInfo, TSharedRef<FUICommandList> InCommandList)
			: ContextName(InContextName), CommandName(InCommandName), CommandInfo(InCommandInfo)
		{
			CommandLists.Add(InCommandList);
		}
	};

	TMap<FName, FCommandListInfo> CommandListInfos;

	struct FToolkitCommandListInfo
	{
		const FName ContextName;
		TSharedRef<FUICommandList> CommandList;

		FToolkitCommandListInfo(const FName InContextName, TSharedRef<FUICommandList> InCommandList)
			: ContextName(InContextName), CommandList(InCommandList)
		{}
	};

	TMap<const void*, FToolkitCommandListInfo> ToolkitCommandListInfos;

	FDelegateHandle RegisterCommandListHandle;

	FDelegateHandle UnregisterCommandListHandle;

	bool bIsInitialized;

	mutable FCriticalSection CommandListsMutex;
};