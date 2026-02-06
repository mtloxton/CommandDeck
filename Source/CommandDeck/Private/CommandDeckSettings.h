// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "CommandDeckSettings.generated.h"

UCLASS(config=Engine, DefaultConfig, Meta = (
	DisplayName = "Command Deck"
))
class UCommandDeckSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, config, Category = "Command Deck")
	FString ServerAddress = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, config, Category = "Command Deck")
	int ServerPort = 47764;

	UPROPERTY(EditAnywhere, config, Category = "Command Deck", Meta = (
		ToolTip = "Automatically connect to Stream Deck when the engine is launched"
	))
	bool bAutoConnect = true;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	virtual FName GetSectionName() const override { return TEXT("Command Deck"); }
};