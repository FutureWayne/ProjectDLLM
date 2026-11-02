// Copyright Ludens Studio. All Rights Reserved.

#include "GameState/ArenaMatchFinaleTypes.h"

#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaMatchFinaleTypes)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameplayCue_MatchFinale_Result, "GameplayCue.UserMessage.MatchFinale.Result");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameplayCue_MatchFinale_FinalScore, "GameplayCue.UserMessage.MatchFinale.FinalScore");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameplayCue_MatchFinale_PlayerStats, "GameplayCue.UserMessage.MatchFinale.PlayerStats");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameplayCue_MatchFinale_NextMatch, "GameplayCue.UserMessage.MatchFinale.NextMatch");

float FArenaMatchFinaleTiming::GetStageSeconds(EArenaMatchFinaleStage Stage) const
{
	switch (Stage)
	{
	case EArenaMatchFinaleStage::Result:
		return ResultSeconds;
	case EArenaMatchFinaleStage::FinalScore:
		return FinalScoreSeconds;
	case EArenaMatchFinaleStage::PlayerStats:
		return PlayerStatsSeconds;
	case EArenaMatchFinaleStage::NextMatch:
		return NextMatchSeconds;
	default:
		return 0.0f;
	}
}

FGameplayTag ArenaMatchFinale::GetStageCueTag(EArenaMatchFinaleStage Stage)
{
	switch (Stage)
	{
	case EArenaMatchFinaleStage::Result:
		return TAG_GameplayCue_MatchFinale_Result;
	case EArenaMatchFinaleStage::FinalScore:
		return TAG_GameplayCue_MatchFinale_FinalScore;
	case EArenaMatchFinaleStage::PlayerStats:
		return TAG_GameplayCue_MatchFinale_PlayerStats;
	case EArenaMatchFinaleStage::NextMatch:
		return TAG_GameplayCue_MatchFinale_NextMatch;
	default:
		return FGameplayTag();
	}
}

EArenaMatchFinaleStage ArenaMatchFinale::GetStageFromCueTag(const FGameplayTag& CueTag)
{
	if (CueTag == TAG_GameplayCue_MatchFinale_Result)
	{
		return EArenaMatchFinaleStage::Result;
	}

	if (CueTag == TAG_GameplayCue_MatchFinale_FinalScore)
	{
		return EArenaMatchFinaleStage::FinalScore;
	}

	if (CueTag == TAG_GameplayCue_MatchFinale_PlayerStats)
	{
		return EArenaMatchFinaleStage::PlayerStats;
	}

	if (CueTag == TAG_GameplayCue_MatchFinale_NextMatch)
	{
		return EArenaMatchFinaleStage::NextMatch;
	}

	return EArenaMatchFinaleStage::None;
}
