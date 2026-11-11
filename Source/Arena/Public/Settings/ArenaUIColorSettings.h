// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/StrongObjectPtr.h"
#include "ArenaUIColorSettings.generated.h"

class UArenaUIColorPalette;

/** Project-wide UI palette, available without a game world for designer previews. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Arena UI Colors"))
class ARENA_API UArenaUIColorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UArenaUIColorSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Arena|UI|Colors")
	TSoftObjectPtr<UArenaUIColorPalette> Palette;

	const UArenaUIColorPalette* GetPalette();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

private:
	// Packaged CDOs can be excluded from GC traversal; retain the asset independently.
	TStrongObjectPtr<UArenaUIColorPalette> LoadedPalette;

	FSoftObjectPath LastAttemptedPath;
	bool bAttemptedLoad = false;
};
