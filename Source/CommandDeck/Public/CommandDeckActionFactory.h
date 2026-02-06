// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "Containers/Map.h"
#include "Templates/Function.h"

#include "CommandDeckAction.h"

using FCommandDeckActionCreator = TFunction<FCommandDeckAction* (const FString& Uuid, const FString& Context)>;

class COMMANDDECK_API FCommandDeckActionFactory
{
public:
	static FCommandDeckActionFactory& Get()
	{
		static FCommandDeckActionFactory Instance;
		return Instance;
	}

	void Register(const FString& InUuid, FCommandDeckActionCreator InCreateFunc)
	{
		ActionCreateFuncMap.Add(InUuid, InCreateFunc);
	}

	FCommandDeckAction* Create(const FString& Uuid, const FString& Context) const
	{
		if (const FCommandDeckActionCreator* Creator = ActionCreateFuncMap.Find(Uuid))
		{
			return (*Creator)(Uuid, Context);
		}
		else
		{
			return new FCommandDeckAction(Uuid, Context);
		}
	}

private:
	TMap<FString, FCommandDeckActionCreator> ActionCreateFuncMap;
};

#define UE_REGISTER_COMMAND_DECK_ACTION(ActionGuid, ActionClass) \
    namespace { \
        struct FAutoRegister_##ActionClass { \
            FAutoRegister_##ActionClass() { \
                FCommandDeckActionFactory::Get().Register(TEXT(ActionGuid), \
                    [](const FString& Uuid, const FString& Context) -> FCommandDeckAction* { \
                        return new ActionClass(Uuid, Context); \
                    }); \
            } \
        }; \
        static FAutoRegister_##ActionClass AutoRegister_##ActionClass; \
    }