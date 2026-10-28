// Copyright Ludens Studio. All Rights Reserved.

#include "UI/ArenaEliminationFeedEntry.h"

namespace ArenaEliminationFeedEntry
{
	bool IsTrailingSurrogate(TCHAR Character)
	{
		return Character >= 0xDC00 && Character <= 0xDFFF;
	}
}

FText UArenaEliminationFeedEntry::ClampDisplayName(const FText& Name, int32 MaxCharacters)
{
	const FString Source = Name.ToString();
	const int32 Limit = FMath::Max(2, MaxCharacters);

	// Keep Limit - 1 code points so the ellipsis brings the total to exactly Limit.
	int32 CodePoints = 0;
	int32 KeepLength = Source.Len();
	for (int32 Index = 0; Index < Source.Len(); ++Index)
	{
		if (ArenaEliminationFeedEntry::IsTrailingSurrogate(Source[Index]))
		{
			continue;
		}

		++CodePoints;
		if (CodePoints == Limit)
		{
			KeepLength = Index;
		}
	}

	if (CodePoints <= Limit)
	{
		return Name;
	}

	return FText::FromString(Source.Left(KeepLength).TrimEnd() + TEXT("\u2026"));
}

void UArenaEliminationFeedEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	ResetEliminationPresentation();
	if (const UArenaEliminationFeedItem* Item = Cast<UArenaEliminationFeedItem>(ListItemObject))
	{
		// The item keeps the full snapshot; only the presented text is shortened.
		PresentElimination(ClampDisplayName(Item->AttackerName, MaxNameLength), ClampDisplayName(Item->VictimName, MaxNameLength),
			Item->AttackerTeamId, Item->VictimTeamId, Item->EliminationType, Item->EliminationStreak);
	}
}

void UArenaEliminationFeedEntry::NativeOnEntryReleased()
{
	ResetEliminationPresentation();
	IUserObjectListEntry::NativeOnEntryReleased();
}
