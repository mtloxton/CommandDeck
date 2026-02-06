// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "UiCommandRegistry.h"

#include "Async/Async.h"
#include "Runtime/Launch/Resources/Version.h"
#include "CommandDeckCoreModule.h"

namespace
{
	FName KMakeCommandKey(const FName InContextName, const FName InCommandName)
	{
		return FName(*FString::Printf(TEXT("%s.%s"), *InContextName.ToString(), *InCommandName.ToString()));
	}
}

FUiCommandRegistry::FUiCommandRegistry()
: RegisterCommandListHandle()
, UnregisterCommandListHandle()
, bIsInitialized(false)
{
}

FUiCommandRegistry& FUiCommandRegistry::Get()
{
	static FUiCommandRegistry Instance;
	return Instance;
}

void FUiCommandRegistry::Startup()
{
	if (!bIsInitialized)
	{
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1)
		RegisterCommandListHandle = FInputBindingManager::Get().OnRegisterCommandList.AddRaw(
			this, &FUiCommandRegistry::RegisterCommandList);

		UnregisterCommandListHandle = FInputBindingManager::Get().OnUnregisterCommandList.AddRaw(
			this, &FUiCommandRegistry::UnregisterCommandList);
#endif
		bIsInitialized = true;
	}
}

void FUiCommandRegistry::Shutdown()
{
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1)
	if (RegisterCommandListHandle.IsValid())
	{
		FInputBindingManager::Get().OnRegisterCommandList.Remove(RegisterCommandListHandle);
		RegisterCommandListHandle.Reset();
	}

	if (UnregisterCommandListHandle.IsValid())
	{
		FInputBindingManager::Get().OnUnregisterCommandList.Remove(UnregisterCommandListHandle);
		UnregisterCommandListHandle.Reset();
	}
#endif

	CommandListInfos.Empty();
	ToolkitCommandListInfos.Empty();

	OnCommandRegistryUpdated.Clear();

	bIsInitialized = false;
}

void FUiCommandRegistry::GetCommands(TArray<TSharedPtr<FUICommandInfo>>& OutCommandInfos) const
{
	for (const TPair<FName, FCommandListInfo>& Kvp : CommandListInfos)
	{
		OutCommandInfos.Add(Kvp.Value.CommandInfo);
	}
}

TSharedPtr<FUICommandInfo> FUiCommandRegistry::GetCommandListsForCommand(const FName InContextName, const FName InCommandName, TArray<TSharedPtr<FUICommandList>>& OutCommandLists) const
{
	FName CommandKey = KMakeCommandKey(InContextName, InCommandName);

	TSharedPtr<FUICommandInfo> CommandInfo = FInputBindingManager::Get().FindCommandInContext(InContextName, InCommandName);

	if (!FInputBindingManager::Get().CommandPassesFilter(InContextName, InCommandName))
	{
		UE_LOG(LogCommandDeckCore, Warning, TEXT("Command did not pass filter: commandContext=%s, commandName=%s"), *InContextName.ToString(), *InCommandName.ToString());
		return CommandInfo;
	}

	{
		FScopeLock Lock(&CommandListsMutex);

		const FCommandListInfo* CommandListInfo = CommandListInfos.Find(CommandKey);
		if (CommandListInfo != nullptr)
		{
			for (TSharedRef<FUICommandList> CommandList : CommandListInfo->CommandLists)
			{
				OutCommandLists.Add(CommandList);
			}
		}
		else
		{
			UE_LOG(LogCommandDeckCore, Verbose, TEXT("No command lists found for command: commandContext=%s, commandName=%s"), *InContextName.ToString(), *InCommandName.ToString());
		}
	}

	return CommandInfo;
}

void FUiCommandRegistry::RegisterToolkitCommandList(const void* InToolkitInstance, const FName InContextName, TSharedRef<FUICommandList> InCommandList)
{
	if (InToolkitInstance == nullptr)
	{
		UE_LOG(LogCommandDeckCore, Warning, TEXT("Cannot register command list for null toolkit: contextName=%s"), *InContextName.ToString());
		return;
	}

	FToolkitCommandListInfo* CommandListInfo = ToolkitCommandListInfos.Find(InToolkitInstance);
	if (CommandListInfo	 == nullptr)
	{
		ToolkitCommandListInfos.Add(InToolkitInstance, FToolkitCommandListInfo(InContextName, InCommandList));
	}
	else
	{
		UE_LOG(LogCommandDeckCore, Warning, TEXT("Toolkit has already registered command list: contextName=%s, toolkit=%p"), *InContextName.ToString(), InToolkitInstance);
	}

	RegisterCommandList(InContextName, InCommandList);

	UE_LOG(LogCommandDeckCore, Log, TEXT("Registered toolkit command list: contextName=%s, toolkit=%p"), *InContextName.ToString(), InToolkitInstance);

}

void FUiCommandRegistry::UnregisterToolkitCommandList(const void* InToolkitInstance)
{
	if (InToolkitInstance == nullptr)
	{
		UE_LOG(LogCommandDeckCore, Warning, TEXT("Cannot unregister command list for null toolkit"));
		return;
	}

	FToolkitCommandListInfo* CommandListInfo = ToolkitCommandListInfos.Find(InToolkitInstance);
	if (CommandListInfo != nullptr)
	{
		ToolkitCommandListInfos.Remove(InToolkitInstance);

		UnregisterCommandList(CommandListInfo->ContextName, CommandListInfo->CommandList);

		UE_LOG(LogCommandDeckCore, Log, TEXT("Unregistered toolkit command list: toolkit=%p"), InToolkitInstance);
	}
	else
	{
		UE_LOG(LogCommandDeckCore, Warning, TEXT("Toolkit has no registered command list to unregister: toolkit=%p"), InToolkitInstance);
	}
}

void FUiCommandRegistry::RegisterCommandList(const FName InContextName, TSharedRef<FUICommandList> InCommandList)
{
	AsyncTask(ENamedThreads::BackgroundThreadPriority, [this, InContextName, InCommandList]()
	{
		TArray<TSharedPtr<FUICommandInfo>> CommandInfos;
		FInputBindingManager::Get().GetCommandInfosFromContext(InContextName, CommandInfos);

		{
			FScopeLock Lock(&CommandListsMutex);

			for (TSharedPtr<FUICommandInfo> CommandInfo : CommandInfos)
			{
				FName CommandName = CommandInfo->GetCommandName();

				FName CommandKey = KMakeCommandKey(InContextName, CommandName);

				FCommandListInfo* CommandListInfo = CommandListInfos.Find(CommandKey);
				if (CommandListInfo != nullptr)
				{
					CommandListInfo->CommandLists.Add(InCommandList);
				}
				else
				{
					CommandListInfos.Add(CommandKey, FCommandListInfo(InContextName, CommandName, CommandInfo, InCommandList));
				}
			}
		}

		UE_LOG(LogCommandDeckCore, Log, TEXT("Registered command list: contextName=%s, commandList=%p"), *InContextName.ToString(), &InCommandList.Get());

		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			OnCommandRegistryUpdated.Broadcast();
		});
	});
}

void FUiCommandRegistry::UnregisterCommandList(const FName InContextName, TSharedRef<FUICommandList> InCommandList)
{
	AsyncTask(ENamedThreads::BackgroundThreadPriority, [this, InContextName, InCommandList]()
	{
		TArray<TSharedPtr<FUICommandInfo>> CommandInfos;
		FInputBindingManager::Get().GetCommandInfosFromContext(InContextName, CommandInfos);

		{
			FScopeLock Lock(&CommandListsMutex);

			for (TSharedPtr<FUICommandInfo> CommandInfo : CommandInfos)
			{
				FName CommandKey = KMakeCommandKey(InContextName, CommandInfo->GetCommandName());

				FCommandListInfo* CommandListInfo = CommandListInfos.Find(CommandKey);
				if (CommandListInfo != nullptr)
				{
					CommandListInfo->CommandLists.Remove(InCommandList);
				}
			}
		}

		UE_LOG(LogCommandDeckCore, Log, TEXT("Unregistered command list: contextName=%s, commandList=%p"), *InContextName.ToString(), &InCommandList.Get());

		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			OnCommandRegistryUpdated.Broadcast();
		});
	});
}
