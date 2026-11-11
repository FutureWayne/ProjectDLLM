// Copyright Ludens Studio. All Rights Reserved.

#include "Settings/ArenaUIColorSettings.h"

#include "ArenaLogChannels.h"
#include "UI/ArenaUIColorPalette.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UArenaUIColorSettings::UArenaUIColorSettings() = default;

const UArenaUIColorPalette* UArenaUIColorSettings::GetPalette()
{
	const FSoftObjectPath PalettePath = Palette.ToSoftObjectPath();
	if (!bAttemptedLoad || LastAttemptedPath != PalettePath)
	{
		bAttemptedLoad = true;
		LastAttemptedPath = PalettePath;
		LoadedPalette.Reset(Palette.LoadSynchronous());
		if (!LoadedPalette)
		{
			UE_LOG(LogArena, Error, TEXT("Failed to load the shared UI palette '%s'. Configure Arena UI Colors in Project Settings."), *PalettePath.ToString());
		}
	}

	return LoadedPalette.Get();
}

#if WITH_EDITOR
EDataValidationResult UArenaUIColorSettings::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	const UArenaUIColorPalette* Asset = Palette.LoadSynchronous();
	if (!Asset)
	{
		Context.AddError(NSLOCTEXT("Arena", "MissingUIColorPalette", "The shared UI palette is missing or cannot be loaded."));
		return EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(Result, Asset->IsDataValid(Context));
}
#endif
