// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Math/Color.h"
#include "ArenaUIColorPalette.generated.h"

UENUM(BlueprintType)
enum class EArenaFactionColorRole : uint8
{
	Primary,
	Highlight,
	Core,
	Secondary,
	Muted,
	Edge,
	Surface,
	Deep
};

USTRUCT(BlueprintType)
struct ARENA_API FArenaFactionPalette
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Primary = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Highlight = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Core = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Secondary = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Muted = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Edge = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Surface = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Deep = FLinearColor::Transparent;

	FLinearColor GetColor(EArenaFactionColorRole Role) const;
};

UENUM(BlueprintType)
enum class EArenaSharedColorRole : uint8
{
	Background,
	Panel,
	Text,
	TextMuted,
	Critical
};

USTRUCT(BlueprintType)
struct ARENA_API FArenaSharedPalette
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Background = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Panel = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Text = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor TextMuted = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FLinearColor Critical = FLinearColor::Transparent;

	FLinearColor GetColor(EArenaSharedColorRole Role) const;
};

/** Shared UI colors. Widgets select roles; visual assets own opacity and animation. */
UCLASS(BlueprintType)
class ARENA_API UArenaUIColorPalette : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FArenaFactionPalette Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FArenaFactionPalette Orange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|UI|Colors")
	FArenaSharedPalette Shared;

	FArenaFactionPalette GetTeamPalette(int32 TeamId) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
