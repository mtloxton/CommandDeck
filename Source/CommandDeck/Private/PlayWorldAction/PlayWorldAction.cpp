// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "PlayWorldAction.h"

#if WITH_EDITOR
#include "LevelEditor.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

#include "CommandDeckModule.h"
#include "CommandDeckPlugin.h"

FCommandDeckPlayWorld::FCommandDeckPlayWorld(const FString& InUuid, const FString& InContext)
	: FCommandDeckAction(InUuid, InContext)
{
#if WITH_EDITOR
	RegisterEventCallback(FName("playInEditor"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckPlayWorld::OnPlayInEditor));

	RegisterEventCallback(FName("endPlay"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckPlayWorld::OnEndPlay));

	BeginPIEDelegateHandle = FEditorDelegates::BeginPIE.AddRaw(this, &FCommandDeckPlayWorld::OnBeginPIE);

	EndPIEDelegateHandle = FEditorDelegates::EndPIE.AddRaw(this, &FCommandDeckPlayWorld::OnEndPIE);

	ensure(GEditor);

	if (GEditor->PlayWorld != NULL)
	{
		OnBeginPIE(false);
	}
#endif
}

FCommandDeckPlayWorld::~FCommandDeckPlayWorld()
{
#if WITH_EDITOR
	if (BeginPIEDelegateHandle.IsValid())
	{
		FEditorDelegates::BeginPIE.Remove(BeginPIEDelegateHandle);
	}

	if (EndPIEDelegateHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPIEDelegateHandle);
	}
#endif
}

void FCommandDeckPlayWorld::OnPlayInEditor(const TSharedPtr<FJsonObject>& InPayload)
{
#if WITH_EDITOR
	ensure(GEditor);

	FRequestPlaySessionParams SessionParams;

	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));

	TSharedPtr<IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();
	if (ActiveLevelViewport.IsValid())
	{
		SessionParams.DestinationSlateViewport = ActiveLevelViewport;
	}

	if (GEditor->PlayWorld == NULL)
	{
		GUnrealEd->RequestPlaySession(SessionParams);
	}
	else
	{
		GEditor->RequestEndPlayMap();
	}
#endif
}

void FCommandDeckPlayWorld::OnEndPlay(const TSharedPtr<FJsonObject>& InPayload)
{
#if WITH_EDITOR
	ensure(GEditor);

	if (GEditor->PlayWorld != NULL)
	{
		GEditor->RequestEndPlayMap();
	}
#endif
}

void FCommandDeckPlayWorld::OnBeginPIE(bool bIsSimulating)
{
	FCommandDeckModule::GetPlugin()->SendToApp(*this, "pieSessionStarted");
}

void FCommandDeckPlayWorld::OnEndPIE(bool bIsSimulating)
{
	FCommandDeckModule::GetPlugin()->SendToApp(*this, "pieSessionEnded");
}