// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "ArenaEliminationFeedEntry.generated.h"

class UArenaGrenadeDefinitionData;

UENUM(BlueprintType)
enum class EArenaEliminationType : uint8
{
	Enemy,
	Ally,
	Self
};

/** Immutable display snapshot; remains valid after a player leaves or respawns. */
UCLASS(BlueprintType)
class ARENA_API UArenaEliminationFeedItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Arena|EliminationFeed")
	FText AttackerName;

	UPROPERTY(BlueprintReadOnly, Category = "Arena|EliminationFeed")
	FText VictimName;

	UPROPERTY(BlueprintReadOnly, Category = "Arena|EliminationFeed")
	int32 AttackerTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Arena|EliminationFeed")
	int32 VictimTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Arena|EliminationFeed")
	EArenaEliminationType EliminationType = EArenaEliminationType::Self;

	/**
	 * Authoritative accolade count for this elimination type: enemy streak (saturated at 6, shown as 6+),
	 * cumulative friendly eliminations (30) or self eliminations (10). Zero when no attacker is known.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Arena|EliminationFeed")
	int32 EliminationStreak = 0;

	/**
	 * Grenade the attacker threw for this elimination, including damage from its submunitions and spawned effects.
	 * Null for non-grenade damage or when attribution is unavailable (e.g. the spawned effect was already destroyed).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Arena|EliminationFeed")
	TObjectPtr<const UArenaGrenadeDefinitionData> GrenadeDefinition = nullptr;

	/** True when another player eliminated the local player that owns the receiving widget. */
	UPROPERTY(BlueprintReadOnly, Category = "Arena|EliminationFeed")
	bool bOwningPlayerEliminated = false;

	double ExpirationTime = 0.0;
};

/** List-entry binding only; the WBP owns text, materials, icons and animation. */
UCLASS(Abstract, Blueprintable)
class ARENA_API UArenaEliminationFeedEntry : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/**
	 * Limits a display name to MaxCharacters code points, replacing the tail with an ellipsis.
	 * Surrogate pairs are never split. Names within the limit are returned unchanged.
	 */
	static FText ClampDisplayName(const FText& Name, int32 MaxCharacters);

	int32 GetMaxNameLength() const { return MaxNameLength; }

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnEntryReleased() override;

	/** Character cap for each name; the row widens to fit names up to this length. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|EliminationFeed", meta = (ClampMin = "2"))
	int32 MaxNameLength = 12;

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|EliminationFeed|Presentation")
	void PresentElimination(const FText& AttackerName, const FText& VictimName, int32 AttackerTeamId, int32 VictimTeamId, EArenaEliminationType EliminationType, int32 EliminationStreak);

	/**
	 * Called instead of PresentElimination when another player eliminated the local player.
	 * The native fallback forwards to PresentElimination, so entries that do not override it keep their usual row.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Arena|EliminationFeed|Presentation")
	void PresentOwningPlayerEliminated(const FText& KillerName, int32 KillerTeamId, const FText& VictimName, int32 VictimTeamId, EArenaEliminationType EliminationType, int32 EliminationStreak);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|EliminationFeed|Presentation")
	void ResetEliminationPresentation();
};
