// Copyright Loxton Enterprises, Inc. All Rights Reserved.

#include "AdjustEditorVolumeAction.h"

#if WITH_EDITOR
#include "LevelEditor.h"
#include "Settings/LevelEditorMiscSettings.h"
#endif
#include "Runtime/Launch/Resources/Version.h"

#include "CommandDeckModule.h"
#include "CommandDeckPlugin.h"

FCommandDeckAdjustEditorVolume::FCommandDeckAdjustEditorVolume(const FString& InUuid, const FString& InContext)
	: FCommandDeckAction(InUuid, InContext)
{
#if WITH_EDITOR
	RegisterEventCallback(FName("setVolume"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckAdjustEditorVolume::OnSetVolume));
	
	RegisterEventCallback(FName("adjustVolume"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckAdjustEditorVolume::OnAdjustVolume));

	RegisterEventCallback(FName("setMute"),
		FCommandDeckEventCallback::CreateRaw(this, &FCommandDeckAdjustEditorVolume::OnSetMute));

	ULevelEditorMiscSettings* LevelEditorMiscSettings = GetMutableDefault<ULevelEditorMiscSettings>();
	UpdateFromSettings(LevelEditorMiscSettings);

	PropertyChangedHandle = LevelEditorMiscSettings->OnSettingChanged().AddLambda(
		[this](UObject* InObject, FPropertyChangedEvent& InEvent)
		{
			if (ULevelEditorMiscSettings* LevelEditorMiscSettings = Cast<ULevelEditorMiscSettings>(InObject))
			{
				this->UpdateFromSettings(LevelEditorMiscSettings);
			}
		}
	);
#endif
}

FCommandDeckAdjustEditorVolume::~FCommandDeckAdjustEditorVolume()
{
#if WITH_EDITOR
	if (PropertyChangedHandle.IsValid() && !IsEngineExitRequested())
	{
		ULevelEditorMiscSettings* LevelEditorMiscSettings = GetMutableDefault<ULevelEditorMiscSettings>();
		LevelEditorMiscSettings->OnSettingChanged().Remove(PropertyChangedHandle);
	}
	PropertyChangedHandle.Reset();
#endif
}

void FCommandDeckAdjustEditorVolume::OnSetVolume(const TSharedPtr<FJsonObject>& InPayload)
{
#if WITH_EDITOR
	ensure(GEditor);

	if (GEditor->IsRealTimeAudioMuted())
	{
		GEditor->MuteRealTimeAudio(false); // Unmute if muted
	}

	double Volume;
	InPayload->TryGetNumberField(TEXT("volume"), Volume);

	GEditor->SetRealTimeAudioVolume(FMath::Clamp(Volume, 0.0, 1.0));
#endif
}

void FCommandDeckAdjustEditorVolume::OnAdjustVolume(const TSharedPtr<FJsonObject>& InPayload)
{
#if WITH_EDITOR
	ensure(GEditor);

	int Steps;
	InPayload->TryGetNumberField(TEXT("steps"), Steps);

	double StepSize;
	InPayload->TryGetNumberField(TEXT("stepSize"), StepSize);

	bool bLinear;
	InPayload->TryGetBoolField(TEXT("isLinear"), bLinear);

	const float CurrentVolume = GEditor->GetRealTimeAudioVolume();

	if (GEditor->IsRealTimeAudioMuted())
	{
		GEditor->MuteRealTimeAudio(false); // Unmute if muted
	}

	if (bLinear)
	{
		const float VolumeDelta = StepSize * Steps;

		const float NewVolume = CurrentVolume + VolumeDelta;

		GEditor->SetRealTimeAudioVolume(FMath::Clamp(NewVolume, 0.0f, 1.0f));
	}
	else
	{
		const float MinDb = -60.0f;
		const float MaxDb = 0.0f;
		const float DbRange = MaxDb - MinDb;
		const float DbDelta = StepSize * Steps;
#if (ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5)
		float CurrentDb = 20.0f * FMath::LogX(10.0f, FMath::Max(CurrentVolume, UE_SMALL_NUMBER));
#else
		float CurrentDb = 20.0f * FMath::LogX(10.0f, FMath::Max(CurrentVolume, SMALL_NUMBER));
#endif
		float NewDb = FMath::Clamp(CurrentDb + DbDelta, MinDb, MaxDb);

		float NewVolume = FMath::Pow(10.0f, NewDb / 20.0f);

		GEditor->SetRealTimeAudioVolume(FMath::Clamp(NewVolume, 0.0f, 1.0f));
	}
#endif
}

void FCommandDeckAdjustEditorVolume::OnSetMute(const TSharedPtr<FJsonObject>& InPayload)
{
#if WITH_EDITOR
	ensure(GEditor);

	bool bMute;
	InPayload->TryGetBoolField(TEXT("isMuted"), bMute);

	GEditor->MuteRealTimeAudio(bMute);
#endif
}

void FCommandDeckAdjustEditorVolume::UpdateFromSettings(const ULevelEditorMiscSettings* InLevelEditorMiscSettings) const
{
#if WITH_EDITOR
	TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();

	Payload->SetBoolField("isMuted", InLevelEditorMiscSettings->bEnableRealTimeAudio ? false : true);
	Payload->SetNumberField("volumeLevel", InLevelEditorMiscSettings->EditorVolumeLevel);

	FCommandDeckModule::GetPlugin()->SendToApp(*this, "volumeInfo", Payload);
#endif
}
