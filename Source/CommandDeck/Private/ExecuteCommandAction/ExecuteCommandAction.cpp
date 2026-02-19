// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "ExecuteCommandAction.h"

#include "ConsoleSettings.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Features/IModularFeatures.h"
#include "Misc/DefaultValueHelper.h"
#include "Runtime/Launch/Resources/Version.h"

#include "CommandDeckModule.h"
#include "CommandDeckPlugin.h"

namespace
{
	bool KAreNumericStringsEqual(const FString& InLhs, const FString& InRhs)
	{
		if (InLhs.Equals(InRhs, ESearchCase::CaseSensitive))
		{
			return true;
		}

		float Value1, Value2;
		if (FDefaultValueHelper::ParseFloat(InLhs, Value1) && FDefaultValueHelper::ParseFloat(InRhs, Value2))
		{
			return FMath::IsNearlyEqual(Value1, Value2);
		}

		return false;
	}

	FString KGetAdjacentValueInList(const FString& InDelimitedList, const FString& InCurrentValue, const FString& InDelimiterValue, bool bInForwardDirection)
	{
		TArray<FString> values;
		InDelimitedList.ParseIntoArray(values, *InDelimiterValue, false);

		if (values.Num() == 0)
		{
			return FString();
		}

		int32 Index = INDEX_NONE;

		for (int32 i = 0; i < values.Num(); ++i)
		{
			if (KAreNumericStringsEqual(values[i], InCurrentValue))
			{
				Index = i;
				break;
			}
		}

		if (Index == INDEX_NONE)
		{
			// If not found, return first or last depending on direction
			return bInForwardDirection ? values[0] : values.Last();
		}

		int32 newIndex;
		if (bInForwardDirection)
		{
			newIndex = (Index + 1) % values.Num(); // Wrap around to start
		}
		else
		{
			newIndex = (Index - 1 + values.Num()) % values.Num(); // Wrap around to end
		}

		return values[newIndex];
	}

	bool KGetCommandProperties(const FString& InCommand, FString& OutType, FString& OutSubType, FString& OutValue, FString& OutHelp)
	{
		if (InCommand.IsEmpty())
			return false;

		IConsoleManager& ConsoleManager = IConsoleManager::Get();

		IConsoleObject* ConsoleObject = ConsoleManager.FindConsoleObject(*InCommand);
		if (ConsoleObject == nullptr)
			return false;

		OutHelp = ConsoleObject->GetHelp();

		IConsoleCommand* ConsoleCommand = ConsoleObject->AsCommand();
		if (ConsoleCommand != nullptr)
		{
			OutType = TEXT("command");
		}
		else
		{
			IConsoleVariable* ConsoleVariable = ConsoleObject->AsVariable();
			if (ConsoleVariable != nullptr)
			{
				OutType = TEXT("variable");

				if (ConsoleVariable->IsVariableBool())
				{
					OutSubType = TEXT("bool");
					OutValue = ConsoleVariable->GetBool() ? TEXT("true") : TEXT("false");
				}
				else if (ConsoleVariable->IsVariableFloat())
				{
					OutSubType = TEXT("float");
					OutValue = FString::SanitizeFloat(ConsoleVariable->GetFloat());
				}
				else if (ConsoleVariable->IsVariableInt())
				{
					OutSubType = TEXT("int");
					OutValue = FString::FromInt(ConsoleVariable->GetInt());
				}
				else if (ConsoleVariable->IsVariableString())
				{
					OutSubType = TEXT("string");
					OutValue = ConsoleVariable->GetString();
				}
				else
				{
					OutSubType = TEXT("bit");
					OutValue = FString::FromInt(ConsoleVariable->GetInt());
				}
			}
			else
			{
				UE_LOG(LogCommandDeck, Warning, TEXT("Unknown console object type: command=%s"), *InCommand);

				return false;
			}
		}

		return true;
	}

	IConsoleCommandExecutor* KGetCommandExecutor(const FName InExecutorName)
	{
		TArray<IConsoleCommandExecutor*> ConsoleCommandExecutors = IModularFeatures::Get().GetModularFeatureImplementations<IConsoleCommandExecutor>(IConsoleCommandExecutor::ModularFeatureName());

		for (IConsoleCommandExecutor* ConsoleCommandExecutor : ConsoleCommandExecutors)
		{
			if (ConsoleCommandExecutor->GetName() == InExecutorName)
				return ConsoleCommandExecutor;
		}

		return nullptr;
	}

	bool KExecuteCommand(const FString& InCommand)
	{
		const FName ExecutorName("Cmd");

		IConsoleCommandExecutor* ConsoleCommandExecutor = KGetCommandExecutor(ExecutorName);

		if (ConsoleCommandExecutor)
		{
			UE_LOG(LogCommandDeck, Log, TEXT("Executing command: executor='%s', command='%s'"), *ExecutorName.ToString(), *InCommand);
			return ConsoleCommandExecutor->Exec(*InCommand);
		}
		else
		{
			UE_LOG(LogCommandDeck, Warning, TEXT("Failed to get command executor: name='%s'"), *InCommand);
		}

		return false;
	}
}

FCommandDeckExecuteCommand::FCommandDeckExecuteCommand(const FString& InUuid, const FString& InContext)
	: FCommandDeckAction(InUuid, InContext)
{
	RegisterEventCallback(FName("execute"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckExecuteCommand::OnExecute));

	RegisterEventCallback(FName("exportCommandData"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckExecuteCommand::OnExportCommandData));

#if WITH_EDITOR
	BeginPIEDelegateHandle = FEditorDelegates::BeginPIE.AddLambda([this](const bool bIsSimulating)
		{
			FCommandDeckModule::GetPlugin()->SendToApp(*this, "beginPIE");
		});

	EndPIEDelegateHandle = FEditorDelegates::EndPIE.AddLambda([this](const bool bIsSimulating)
		{
			FCommandDeckModule::GetPlugin()->SendToApp(*this, "endPIE");
		});
#endif
}


FCommandDeckExecuteCommand::~FCommandDeckExecuteCommand()
{
#if WITH_EDITOR
	if (BeginPIEDelegateHandle.IsValid())
	{
		FEditorDelegates::BeginPIE.Remove(BeginPIEDelegateHandle);
		BeginPIEDelegateHandle.Reset();
	}

	if (EndPIEDelegateHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPIEDelegateHandle);
		EndPIEDelegateHandle.Reset();
	}
#endif
}

void FCommandDeckExecuteCommand::OnExecute(const TSharedPtr<FJsonObject>& InPayload)
{
	FString Command = InPayload->GetStringField(TEXT("command"));

	FString Sequence = InPayload->GetStringField(TEXT("sequence"));

	bool bHandled = false;

	FString OutType, OutSubType, OutValue, OutHelp;

	if (!Sequence.IsEmpty())
	{
		bool bResult = KGetCommandProperties(Command, OutType, OutSubType, OutValue, OutHelp);

		if (OutValue.IsEmpty()) // Is the current value unknown?
		{	
			OutValue = CachedValue; // Use the cached value from last time the command was executed by this action
		}

		FString Value = KGetAdjacentValueInList(Sequence, OutValue, ";", true);

		bHandled = KExecuteCommand(Command + TEXT(" ") + Value);
		if (bHandled)
		{
			OutValue = CachedValue = Value; // Cache the applied value
		}
	}
	else
	{
		bHandled = KExecuteCommand(Command);
	}

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetBoolField("handled", bHandled);
	if (!OutValue.IsEmpty())
	{
		Payload->SetStringField("value", OutValue);
	}

	FCommandDeckModule::GetPlugin()->SendToApp(*this, "commandResult", Payload);
}

void FCommandDeckExecuteCommand::OnExportCommandData(const TSharedPtr<FJsonObject>& InPayload)
{
	FString Command = InPayload->GetStringField(TEXT("command"));

	TArray<TSharedPtr<FJsonValue>> Suggestions;

	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();

	if (!Command.IsEmpty())
	{
		IConsoleManager& ConsoleManager = IConsoleManager::Get();

		IConsoleObject* ConsoleObject = ConsoleManager.FindConsoleObject(*Command);
		if (ConsoleObject != nullptr)
		{
			Payload->SetStringField("command", *Command);
			Payload->SetStringField("help", ConsoleObject->GetHelp());

			IConsoleCommand* ConsoleCommand = ConsoleObject->AsCommand();
			if (ConsoleCommand != nullptr)
			{
				Payload->SetStringField("type", "command");
			}
			else
			{
				IConsoleVariable* ConsoleVariable = ConsoleObject->AsVariable();
				if (ConsoleVariable != nullptr)
				{
					Payload->SetStringField("type", "variable");

					if (ConsoleVariable->IsVariableBool())
					{
						Payload->SetStringField("subtype", "bool");
						Payload->SetStringField("value", ConsoleVariable->GetBool() ? TEXT("true") : TEXT("false"));
					}
					else if (ConsoleVariable->IsVariableFloat())
					{
						Payload->SetStringField("subtype", "float");
						Payload->SetNumberField("value", ConsoleVariable->GetFloat());
					}
					else if (ConsoleVariable->IsVariableInt())
					{
						Payload->SetStringField("subtype", "int");
						Payload->SetNumberField("value", ConsoleVariable->GetInt());
					}
					else if (ConsoleVariable->IsVariableString())
					{
						Payload->SetStringField("subtype", "string");
						Payload->SetStringField("value", ConsoleVariable->GetString());
					}
					else
					{
						Payload->SetStringField("subtype", "bit");
						Payload->SetNumberField("value", ConsoleVariable->GetInt());
					}
				}
				else
				{
					Payload->SetStringField("type", "unknown");

					UE_LOG(LogCommandDeck, Warning, TEXT("Unknown console object type: command=%s"), *Command);
				}
			}
		}
		else
		{
			// ConsoleSettings are not considered ConsoleObjects, and so are checked seperately 
			auto ManualAutoCompleteList = GetDefault<UConsoleSettings>()->ManualAutoCompleteList;
			TArray<FAutoCompleteCommand> filteredResults = ManualAutoCompleteList.FilterByPredicate([Command](const FAutoCompleteCommand InAutoCompleteCommand)
				{
					return InAutoCompleteCommand.Command == Command;
				});

			if (filteredResults.Num() == 1)
			{
				Payload->SetStringField("command", *Command);
				Payload->SetStringField("type", "setting");
				Payload->SetStringField("help", filteredResults[0].Desc);
			}
		}

		const FName ExecutorName("Cmd");

		IConsoleCommandExecutor* CommandExecutor = KGetCommandExecutor(ExecutorName);
		if (CommandExecutor != nullptr)
		{
			// GetAutoCompleteSuggestions was deprecated in UE 5.5
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5)
			TArray<FConsoleSuggestion> ConsoleSuggestions;
			CommandExecutor->GetSuggestedCompletions(*Command, ConsoleSuggestions);

			ConsoleSuggestions.Sort([this, Command](const FConsoleSuggestion& InLhs, const FConsoleSuggestion& InRhs)
				{
					if (InLhs.Name.StartsWith(Command))
					{
						if (!InRhs.Name.StartsWith(Command))
							return true;
					}
					else
					{
						if (InRhs.Name.StartsWith(Command))
							return false;
					}

					return InLhs.Name < InRhs.Name;
				});

			for (int32 i = 0; i < ConsoleSuggestions.Num(); ++i)
			{
				Suggestions.Add(MakeShared<FJsonValueString>(ConsoleSuggestions[i].Name));
			}
#else
			TArray<FString> ConsoleSuggestions;
			CommandExecutor->GetAutoCompleteSuggestions(*Command, ConsoleSuggestions);

			ConsoleSuggestions.Sort([Command](const FString& lhs, const FString& rhs)
				{
					if (lhs.StartsWith(Command))
					{
						if (!rhs.StartsWith(Command))
							return true;
					}
					else
					{
						if (rhs.StartsWith(Command))
							return false;
					}

					return lhs < rhs;
				});

			for (int32 i = 0; i < ConsoleSuggestions.Num(); ++i)
			{
				Suggestions.Add(MakeShared<FJsonValueString>(ConsoleSuggestions[i]));
			}
#endif
		}
	}

	Payload->SetArrayField("suggestions", Suggestions);

	FCommandDeckModule::GetPlugin()->SendToApp(*this, "commandData", Payload);
}