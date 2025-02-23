// Copyright Ludens Studio. All Rights Reserved.

#include "Settings/ArenaGameSettingRegistry.h"

#include "GameSettingCollection.h"
#include "Settings/ArenaSettingsLocal.h"
#include "Settings/ArenaSettingsShared.h"
#include "Player/ArenaLocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaGameSettingRegistry)

DEFINE_LOG_CATEGORY(LogArenaGameSettingRegistry);

#define LOCTEXT_NAMESPACE "Arena"

//--------------------------------------
// UArenaGameSettingRegistry
//--------------------------------------

UArenaGameSettingRegistry::UArenaGameSettingRegistry()
{
}

UArenaGameSettingRegistry* UArenaGameSettingRegistry::Get(UArenaLocalPlayer* InLocalPlayer)
{
	UArenaGameSettingRegistry* Registry = FindObject<UArenaGameSettingRegistry>(InLocalPlayer, TEXT("ArenaGameSettingRegistry"), true);
	if (Registry == nullptr)
	{
		Registry = NewObject<UArenaGameSettingRegistry>(InLocalPlayer, TEXT("ArenaGameSettingRegistry"));
		Registry->Initialize(InLocalPlayer);
	}

	return Registry;
}

bool UArenaGameSettingRegistry::IsFinishedInitializing() const
{
	if (Super::IsFinishedInitializing())
	{
		if (UArenaLocalPlayer* LocalPlayer = Cast<UArenaLocalPlayer>(OwningLocalPlayer))
		{
			if (LocalPlayer->GetSharedSettings() == nullptr)
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void UArenaGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	UArenaLocalPlayer* ArenaLocalPlayer = Cast<UArenaLocalPlayer>(InLocalPlayer);

	VideoSettings = InitializeVideoSettings(ArenaLocalPlayer);
	InitializeVideoSettings_FrameRates(VideoSettings, ArenaLocalPlayer);
	RegisterSetting(VideoSettings);
	
	AudioSettings = InitializeAudioSettings(ArenaLocalPlayer);
	RegisterSetting(AudioSettings);
	
	GameplaySettings = InitializeGameplaySettings(ArenaLocalPlayer);
	RegisterSetting(GameplaySettings);
	
	MouseAndKeyboardSettings = InitializeMouseAndKeyboardSettings(ArenaLocalPlayer);
	RegisterSetting(MouseAndKeyboardSettings);
	
	GamepadSettings = InitializeGamepadSettings(ArenaLocalPlayer);
	RegisterSetting(GamepadSettings);
}

void UArenaGameSettingRegistry::SaveChanges()
{
	Super::SaveChanges();
	
	if (UArenaLocalPlayer* LocalPlayer = Cast<UArenaLocalPlayer>(OwningLocalPlayer))
	{
		// Game user settings need to be applied to handle things like resolution, this saves indirectly
		LocalPlayer->GetLocalSettings()->ApplySettings(false);
		
		LocalPlayer->GetSharedSettings()->ApplySettings();
		LocalPlayer->GetSharedSettings()->SaveSettings();
	}
}

#undef LOCTEXT_NAMESPACE


