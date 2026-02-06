// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "ExecuteUiCommandAction.h"

#include "ConsoleSettings.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Features/IModularFeatures.h"
#include "Misc/DefaultValueHelper.h"
#include "Runtime/Launch/Resources/Version.h"

#include "CommandDeckModule.h"
#include "CommandDeckPlugin.h"
#include "UiCommandRegistry.h"

namespace
{
	bool KExecuteCommand(const FName InContextName, const FName InCommandName)
	{
		UE_LOG(LogCommandDeck, Log, TEXT("Executing command: context='%s', command='%s'"), *InContextName.ToString(), *InCommandName.ToString());

		TArray<TSharedPtr<FUICommandList>> CommandLists;
		TSharedPtr<FUICommandInfo> CommandInfo = FUiCommandRegistry::Get().GetCommandListsForCommand(InContextName, InCommandName, CommandLists);

		if (!CommandInfo.IsValid())
		{
			return false; // OK to silently fail here
		}

		bool bSuccess = false;

		for (const TSharedPtr<FUICommandList>& CommandList : CommandLists)
		{
			if (!CommandList.IsValid())
			{
				continue;
			}

			const FUIAction* Action = CommandList->GetActionForCommand(CommandInfo);
			if (Action == nullptr)
			{
				continue;
			}

			if (Action->CanExecuteAction.IsBound())
			{
				if (Action->CanExecute())
				{
					bSuccess |= Action->Execute();
				}
			}
			else
			{
				if (Action->ExecuteAction.IsBound())
				{
					bSuccess |= Action->Execute();
				}
			}

			if (bSuccess)
				break;
		}

		return bSuccess;
	}
}

FCommandDeckExecuteUiCommand::FCommandDeckExecuteUiCommand(const FString& InUuid, const FString& InContext)
	: FCommandDeckAction(InUuid, InContext)
{
	RegisterEventCallback(FName("execute"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckExecuteUiCommand::OnExecute));

	RegisterEventCallback(FName("exportCommandList"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckExecuteUiCommand::OnExportCommandList));

	RegisterEventCallback(FName("exportCommandInfo"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckExecuteUiCommand::OnExportCommandInfo));

	CommandRegisteryUpdatedHandle = FUiCommandRegistry::Get().OnCommandRegistryUpdated.AddRaw(this,
		&FCommandDeckExecuteUiCommand::OnCommandRegisteryUpdated);
}

FCommandDeckExecuteUiCommand::~FCommandDeckExecuteUiCommand()
{
	if (CommandRegisteryUpdatedHandle.IsValid())
	{
		FUiCommandRegistry::Get().OnCommandRegistryUpdated.Remove(CommandRegisteryUpdatedHandle);
		CommandRegisteryUpdatedHandle.Reset();
	}
}

void FCommandDeckExecuteUiCommand::OnCommandRegisteryUpdated()
{
	FCommandDeckModule::GetPlugin()->SendToApp(*this, "commandRegistryUpdated");
}

void FCommandDeckExecuteUiCommand::OnExecute(const TSharedPtr<FJsonObject>& InPayload)
{
	FName InCommand(InPayload->GetStringField(TEXT("command")));
	FName InContext(InPayload->GetStringField(TEXT("context")));

	bool bSuccess = KExecuteCommand(InContext, InCommand);

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetBoolField(TEXT("success"), bSuccess);

	FCommandDeckModule::GetPlugin()->SendToApp(*this, "commandResult", Payload);
}

void FCommandDeckExecuteUiCommand::OnExportCommandList(const TSharedPtr<FJsonObject>& InPayload)
{
	TArray<TSharedPtr<FUICommandInfo>> CommandInfos;
	FUiCommandRegistry::Get().GetCommands(CommandInfos);

	TArray<TSharedPtr<FJsonValue>> Commands;

	for (const TSharedPtr<FUICommandInfo>& CommandInfo : CommandInfos)
	{
		TSharedPtr<FJsonObject> CommandJson = MakeShared<FJsonObject>();
		CommandJson->SetStringField(TEXT("commandName"), CommandInfo->GetCommandName().ToString());
		CommandJson->SetStringField(TEXT("commandContext"), CommandInfo->GetBindingContext().ToString());
		CommandJson->SetStringField(TEXT("commandLabel"), CommandInfo->GetLabel().ToString());

		Commands.Add(MakeShared<FJsonValueObject>(MoveTemp(CommandJson)));
	}

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetArrayField(TEXT("commands"), Commands);

	FCommandDeckModule::GetPlugin()->SendToApp(*this, "commandList", Payload);
}

void FCommandDeckExecuteUiCommand::OnExportCommandInfo(const TSharedPtr<FJsonObject>& InPayload)
{
	FName ContextName(InPayload->GetStringField(TEXT("context")));
	FName CommandName(InPayload->GetStringField(TEXT("command")));

	TArray<TSharedPtr<FUICommandList>> CommandLists;
	TSharedPtr<FUICommandInfo> CommandInfo = FUiCommandRegistry::Get().GetCommandListsForCommand(ContextName, CommandName, CommandLists);

	if (!CommandInfo.IsValid())
	{
		return;
	}

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("description"), CommandInfo->GetDescription().ToString());
	Payload->SetStringField(TEXT("label"), CommandInfo->GetLabel().ToString());
	Payload->SetNumberField(TEXT("bindingCount"), CommandLists.Num());

	const FSlateIcon& Icon = CommandInfo->GetIcon();
	if (Icon.IsSet())
	{
		const FSlateBrush* IconBrush = Icon.GetIcon();
		if (IconBrush && !IconBrush->GetResourceName().IsNone())
		{
			Payload->SetStringField(TEXT("resourceName"), IconBrush->GetResourceName().ToString());
		}
	}

	FCommandDeckModule::GetPlugin()->SendToApp(*this, "commandInfo", Payload);
}
