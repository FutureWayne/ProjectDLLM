// Copyright Ludens Studio. All Rights Reserved.

#include "UI/ArenaUIColorLibrary.h"

#include "Settings/ArenaUIColorSettings.h"

FArenaFactionPalette UArenaUIColorLibrary::GetTeamPalette(int32 TeamId)
{
	const UArenaUIColorPalette* Palette = GetMutableDefault<UArenaUIColorSettings>()->GetPalette();
	return Palette ? Palette->GetTeamPalette(TeamId) : FArenaFactionPalette();
}

FLinearColor UArenaUIColorLibrary::GetTeamColor(int32 TeamId, EArenaFactionColorRole Role)
{
	return GetTeamPalette(TeamId).GetColor(Role);
}

FArenaSharedPalette UArenaUIColorLibrary::GetSharedPalette()
{
	const UArenaUIColorPalette* Palette = GetMutableDefault<UArenaUIColorSettings>()->GetPalette();
	return Palette ? Palette->Shared : FArenaSharedPalette();
}

FLinearColor UArenaUIColorLibrary::GetSharedColor(EArenaSharedColorRole Role)
{
	return GetSharedPalette().GetColor(Role);
}
