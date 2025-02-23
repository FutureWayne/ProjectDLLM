// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/CustomSettings/ArenaSettingValueDiscrete_MobileFPSType.h"

#include "Performance/ArenaPerformanceSettings.h"
#include "Settings/ArenaSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaSettingValueDiscrete_MobileFPSType)

#define LOCTEXT_NAMESPACE "ArenaSettings"

UArenaSettingValueDiscrete_MobileFPSType::UArenaSettingValueDiscrete_MobileFPSType()
{
}

void UArenaSettingValueDiscrete_MobileFPSType::OnInitialized()
{
	Super::OnInitialized();

	const UArenaPlatformSpecificRenderingSettings* PlatformSettings = UArenaPlatformSpecificRenderingSettings::Get();
	const UArenaSettingsLocal* UserSettings = UArenaSettingsLocal::Get();

	for (int32 TestLimit : PlatformSettings->MobileFrameRateLimits)
	{
		if (UArenaSettingsLocal::IsSupportedMobileFramePace(TestLimit))
		{
			FPSOptions.Add(TestLimit, MakeLimitString(TestLimit));
		}
	}

	const int32 FirstFrameRateWithQualityLimit = UserSettings->GetFirstFrameRateWithQualityLimit();
	if (FirstFrameRateWithQualityLimit > 0)
	{
		SetWarningRichText(FText::Format(LOCTEXT("MobileFPSType_Note", "<strong>Note: Changing the framerate setting to {0} or higher might lower your Quality Presets.</>"), MakeLimitString(FirstFrameRateWithQualityLimit)));
	}
}

int32 UArenaSettingValueDiscrete_MobileFPSType::GetDefaultFPS() const
{
	return UArenaSettingsLocal::GetDefaultMobileFrameRate();
}

FText UArenaSettingValueDiscrete_MobileFPSType::MakeLimitString(int32 Number)
{
	return FText::Format(LOCTEXT("MobileFrameRateOption", "{0} FPS"), FText::AsNumber(Number));
}

void UArenaSettingValueDiscrete_MobileFPSType::StoreInitial()
{
	InitialValue = GetValue();
}

void UArenaSettingValueDiscrete_MobileFPSType::ResetToDefault()
{
	SetValue(GetDefaultFPS(), EGameSettingChangeReason::ResetToDefault);
}

void UArenaSettingValueDiscrete_MobileFPSType::RestoreToInitial()
{
	SetValue(InitialValue, EGameSettingChangeReason::RestoreToInitial);
}

void UArenaSettingValueDiscrete_MobileFPSType::SetDiscreteOptionByIndex(int32 Index)
{
	TArray<int32> FPSOptionsModes;
	FPSOptions.GenerateKeyArray(FPSOptionsModes);

	int32 NewMode = FPSOptionsModes.IsValidIndex(Index) ? FPSOptionsModes[Index] : GetDefaultFPS();

	SetValue(NewMode, EGameSettingChangeReason::Change);
}

int32 UArenaSettingValueDiscrete_MobileFPSType::GetDiscreteOptionIndex() const
{
	TArray<int32> FPSOptionsModes;
	FPSOptions.GenerateKeyArray(FPSOptionsModes);
	return FPSOptionsModes.IndexOfByKey(GetValue());
}

TArray<FText> UArenaSettingValueDiscrete_MobileFPSType::GetDiscreteOptions() const
{
	TArray<FText> Options;
	FPSOptions.GenerateValueArray(Options);

	return Options;
}

int32 UArenaSettingValueDiscrete_MobileFPSType::GetValue() const
{
	return UArenaSettingsLocal::Get()->GetDesiredMobileFrameRateLimit();
}

void UArenaSettingValueDiscrete_MobileFPSType::SetValue(int32 NewLimitFPS, EGameSettingChangeReason InReason)
{
	UArenaSettingsLocal::Get()->SetDesiredMobileFrameRateLimit(NewLimitFPS);

	NotifySettingChanged(InReason);
}

#undef LOCTEXT_NAMESPACE

