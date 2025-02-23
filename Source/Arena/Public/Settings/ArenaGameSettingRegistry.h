// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameSettingRegistry.h"

#include "ArenaGameSettingRegistry.generated.h"

class ULocalPlayer;
class UObject;

//--------------------------------------
// UArenaGameSettingRegistry
//--------------------------------------

class UGameSettingCollection;
class UArenaLocalPlayer;

DECLARE_LOG_CATEGORY_EXTERN(LogArenaGameSettingRegistry, Log, Log);

#define GET_SHARED_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)							\
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({								\
		GET_FUNCTION_NAME_STRING_CHECKED(UArenaLocalPlayer, GetSharedSettings),				\
		GET_FUNCTION_NAME_STRING_CHECKED(UArenaSettingsShared, FunctionOrPropertyName)		\
	}))

#define GET_LOCAL_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)							\
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({								\
		GET_FUNCTION_NAME_STRING_CHECKED(UArenaLocalPlayer, GetLocalSettings),				\
		GET_FUNCTION_NAME_STRING_CHECKED(UArenaSettingsLocal, FunctionOrPropertyName)		\
	}))

/**
 * 
 */
UCLASS()
class UArenaGameSettingRegistry : public UGameSettingRegistry
{
	GENERATED_BODY()

public:
	UArenaGameSettingRegistry();

	static UArenaGameSettingRegistry* Get(UArenaLocalPlayer* InLocalPlayer);
	
	virtual void SaveChanges() override;

protected:
	virtual void OnInitialize(ULocalPlayer* InLocalPlayer) override;
	virtual bool IsFinishedInitializing() const override;

	// UGameSettingCollection* InitializeVideoSettings(UArenaLocalPlayer* InLocalPlayer);
	// void InitializeVideoSettings_FrameRates(UGameSettingCollection* Screen, UArenaLocalPlayer* InLocalPlayer);
	// void AddPerformanceStatPage(UGameSettingCollection* Screen, UArenaLocalPlayer* InLocalPlayer);
	//
	// UGameSettingCollection* InitializeAudioSettings(UArenaLocalPlayer* InLocalPlayer);
	// UGameSettingCollection* InitializeGameplaySettings(UArenaLocalPlayer* InLocalPlayer);
	//
	// UGameSettingCollection* InitializeMouseAndKeyboardSettings(UArenaLocalPlayer* InLocalPlayer);
	// UGameSettingCollection* InitializeGamepadSettings(UArenaLocalPlayer* InLocalPlayer);

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> VideoSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> AudioSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> GameplaySettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> MouseAndKeyboardSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> GamepadSettings;
};
