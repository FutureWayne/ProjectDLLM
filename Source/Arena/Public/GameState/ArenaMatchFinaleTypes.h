// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ArenaMatchFinaleTypes.generated.h"

/** Ordered stages of the post-match finale. None means no finale is running. */
UENUM(BlueprintType)
enum class EArenaMatchFinaleStage : uint8
{
	None,
	Result,
	FinalScore,
	PlayerStats,
	NextMatch
};

/** Real-time duration of every finale stage, in seconds. */
USTRUCT(BlueprintType)
struct ARENA_API FArenaMatchFinaleTiming
{
	GENERATED_BODY()

	/** Includes the slow-motion lead-in before the result is revealed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Finale", meta = (ClampMin = "0.0", Units = "s"))
	float ResultSeconds = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Finale", meta = (ClampMin = "0.0", Units = "s"))
	float FinalScoreSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Finale", meta = (ClampMin = "0.0", Units = "s"))
	float PlayerStatsSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Finale", meta = (ClampMin = "0.0", Units = "s"))
	float NextMatchSeconds = 3.0f;

	float GetStageSeconds(EArenaMatchFinaleStage Stage) const;
};

/**
 * Finale values every client reads from the GameState. Stages themselves arrive through GameplayCues.
 * A WinningTeamId of 0 or less is a draw, matching the match scoring Blueprint (1 = red, 2 = blue).
 */
USTRUCT(BlueprintType)
struct ARENA_API FArenaMatchFinaleData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Arena|Finale")
	bool bActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Arena|Finale")
	int32 WinningTeamId = 0;

	/** Team scores frozen when the finale begins. */
	UPROPERTY(BlueprintReadOnly, Category = "Arena|Finale")
	int32 BlueScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Arena|Finale")
	int32 RedScore = 0;

	/** Server world time the current stage began; only used to place a player who joins mid-stage. */
	UPROPERTY(BlueprintReadOnly, Category = "Arena|Finale")
	double StageStartServerTime = 0.0;

	/** Identifies the stage the timestamp belongs to; does not advance the local stage. */
	UPROPERTY()
	EArenaMatchFinaleStage TimingStage = EArenaMatchFinaleStage::None;
};

namespace ArenaMatchFinale
{
	// Team ids used by the TDM scoring Blueprint and the TopBar.
	inline constexpr int32 RedTeamId = 1;
	inline constexpr int32 BlueTeamId = 2;

	/**
	 * Looping GameplayCue that marks a stage as current on the GameState. These tags have no notify asset:
	 * AArenaGameState receives them through IGameplayCueInterface. Invalid for stages without a cue.
	 */
	ARENA_API FGameplayTag GetStageCueTag(EArenaMatchFinaleStage Stage);

	/** Returns None when the tag is not a finale stage cue. */
	ARENA_API EArenaMatchFinaleStage GetStageFromCueTag(const FGameplayTag& CueTag);
}
