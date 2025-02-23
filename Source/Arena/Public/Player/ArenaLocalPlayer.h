// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CommonLocalPlayer.h"
#include "Teams/ArenaTeamAgentInterface.h"
#include "ArenaLocalPlayer.generated.h"

class UArenaSettingsLocal;
class UArenaSettingsShared;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaLocalPlayer : public UCommonLocalPlayer, public IArenaTeamAgentInterface
{
	GENERATED_BODY()

public:
	UArenaLocalPlayer();
	
	//~ Begin IArenaTeamAgentInterface Interface
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnArenaTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~ End IArenaTeamAgentInterface Interface

	/** Gets the local settings for this player, this is read from config files at process startup and is always valid */
	UFUNCTION()
	UArenaSettingsLocal* GetLocalSettings() const;

	/** Gets the shared setting for this player, this is read using the save game system so may not be correct until after user login */
	UFUNCTION()
	UArenaSettingsShared* GetSharedSettings() const;

	/** Starts an async request to load the shared settings, this will call OnSharedSettingsLoaded after loading or creating new ones */
	void LoadSharedSettingsFromDisk(bool bForceLoad = false);

protected:
	void OnSharedSettingsLoaded(UArenaSettingsShared* LoadedOrCreatedSettings);

private:
	UPROPERTY(Transient)
	mutable TObjectPtr<UArenaSettingsShared> SharedSettings;

	FUniqueNetIdRepl NetIdForSharedSettings;
	
	UPROPERTY()
    FOnArenaTeamIndexChangedDelegate OnTeamChangedDelegate;
};
