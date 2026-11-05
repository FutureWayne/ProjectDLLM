// Copyright Ludens Studio. All Rights Reserved.

#include "UI/ArenaUIColorPalette.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FLinearColor FArenaFactionPalette::GetColor(EArenaFactionColorRole Role) const
{
	switch (Role)
	{
	case EArenaFactionColorRole::Primary:
		return Primary;
	case EArenaFactionColorRole::Highlight:
		return Highlight;
	case EArenaFactionColorRole::Core:
		return Core;
	case EArenaFactionColorRole::Secondary:
		return Secondary;
	case EArenaFactionColorRole::Muted:
		return Muted;
	case EArenaFactionColorRole::Edge:
		return Edge;
	case EArenaFactionColorRole::Surface:
		return Surface;
	case EArenaFactionColorRole::Deep:
		return Deep;
	default:
		return FLinearColor::Transparent;
	}
}

FLinearColor FArenaSharedPalette::GetColor(EArenaSharedColorRole Role) const
{
	switch (Role)
	{
	case EArenaSharedColorRole::Background:
		return Background;
	case EArenaSharedColorRole::Panel:
		return Panel;
	case EArenaSharedColorRole::Text:
		return Text;
	case EArenaSharedColorRole::TextMuted:
		return TextMuted;
	default:
		return FLinearColor::Transparent;
	}
}

FArenaFactionPalette UArenaUIColorPalette::GetTeamPalette(int32 TeamId) const
{
	if (TeamId == 1)
	{
		return Orange;
	}

	if (TeamId == 2)
	{
		return Cyan;
	}

	FArenaFactionPalette Neutral;
	Neutral.Primary = Shared.Text;
	Neutral.Highlight = Shared.Text;
	Neutral.Core = Shared.Text;
	Neutral.Secondary = Shared.TextMuted;
	Neutral.Muted = Shared.TextMuted;
	Neutral.Edge = Shared.Panel;
	Neutral.Surface = Shared.Panel;
	Neutral.Deep = Shared.Background;
	return Neutral;
}

#if WITH_EDITOR
EDataValidationResult UArenaUIColorPalette::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	const FLinearColor Colors[] =
	{
		Cyan.Primary,
		Cyan.Highlight,
		Cyan.Core,
		Cyan.Secondary,
		Cyan.Muted,
		Cyan.Edge,
		Cyan.Surface,
		Cyan.Deep,
		Orange.Primary,
		Orange.Highlight,
		Orange.Core,
		Orange.Secondary,
		Orange.Muted,
		Orange.Edge,
		Orange.Surface,
		Orange.Deep,
		Shared.Background,
		Shared.Panel,
		Shared.Text,
		Shared.TextMuted,
	};

	for (const FLinearColor& Color : Colors)
	{
		if (!FMath::IsFinite(Color.R) || !FMath::IsFinite(Color.G) || !FMath::IsFinite(Color.B) ||
			Color.R < 0.0f || Color.R > 1.0f || Color.G < 0.0f || Color.G > 1.0f ||
			Color.B < 0.0f || Color.B > 1.0f || Color.A != 1.0f)
		{
			Context.AddError(NSLOCTEXT("Arena", "InvalidUIColorPalette", "UI palette colors must contain finite linear RGB values between zero and one, with alpha equal to one."));
			Result = EDataValidationResult::Invalid;
			break;
		}
	}

	return Result;
}
#endif
