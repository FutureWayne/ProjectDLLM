// Copyright Ludens Studio. All Rights Reserved.


#include "Player/ArenaLocalPlayer.h"

#include "Settings/ArenaSettingsLocal.h"
#include "Settings/ArenaSettingsShared.h"

UArenaLocalPlayer::UArenaLocalPlayer()
{
}

void UArenaLocalPlayer::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	// Do nothing, we merely observe the team of our associated player controller
}

FGenericTeamId UArenaLocalPlayer::GetGenericTeamId() const
{
	if (IArenaTeamAgentInterface* ControllerAsTeamProvider = Cast<IArenaTeamAgentInterface>(PlayerController))
	{
		return ControllerAsTeamProvider->GetGenericTeamId();
	}
	else
	{
		return FGenericTeamId::NoTeam;
	}
}

FOnArenaTeamIndexChangedDelegate* UArenaLocalPlayer::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

UArenaSettingsLocal* UArenaLocalPlayer::GetLocalSettings() const
{
	return UArenaSettingsLocal::Get();
}

UArenaSettingsShared* UArenaLocalPlayer::GetSharedSettings() const
{
	if (!SharedSettings)
	{
		// On PC it's okay to use the sync load because it only checks the disk
		// This could use a platform tag to check for proper save support instead
		bool bCanLoadBeforeLogin = PLATFORM_DESKTOP;
		
		if (bCanLoadBeforeLogin)
		{
			SharedSettings = UArenaSettingsShared::LoadOrCreateSettings(this);
		}
		else
		{
			// We need to wait for user login to get the real settings so return temp ones
			SharedSettings = UArenaSettingsShared::CreateTemporarySettings(this);
		}
	}

	return SharedSettings;
}

void UArenaLocalPlayer::LoadSharedSettingsFromDisk(bool bForceLoad)
{
	FUniqueNetIdRepl CurrentNetId = GetCachedUniqueNetId();
	if (!bForceLoad && SharedSettings && CurrentNetId == NetIdForSharedSettings)
	{
		// Already loaded once, don't reload
		return;
	}

	ensure(UArenaSettingsShared::AsyncLoadOrCreateSettings(this, UArenaSettingsShared::FOnSettingsLoadedEvent::CreateUObject(this, &UArenaLocalPlayer::OnSharedSettingsLoaded)));
}

void UArenaLocalPlayer::OnSharedSettingsLoaded(UArenaSettingsShared* LoadedOrCreatedSettings)
{
	// The settings are applied before it gets here
	if (ensure(LoadedOrCreatedSettings))
	{
		// This will replace the temporary or previously loaded object which will GC out normally
		SharedSettings = LoadedOrCreatedSettings;

		NetIdForSharedSettings = GetCachedUniqueNetId();
	}
}
