// Copyright Ludens Studio. All Rights Reserved.

#include "AbilitySystem/Phases/ArenaGamePhaseAbility_MatchFinale.h"

#include "ArenaLogChannels.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffectTypes.h"
#include "GameState/ArenaGameState.h"
#include "NativeGameplayTags.h"
#include "Teams/ArenaTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaGamePhaseAbility_MatchFinale)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameplayCue_MatchDecided_Finale, "GameplayCue.UserMessage.MatchDecided");

void UArenaGamePhaseAbility_MatchFinale::BeginMatchFinale(int32 WinningTeamId)
{
	AArenaGameState* GameState = Cast<AArenaGameState>(GetOwningActorFromActorInfo());
	if (GameState == nullptr)
	{
		UE_LOG(LogArena, Warning, TEXT("BeginMatchFinale: %s is not owned by an AArenaGameState; scores are not published"), *GetNameSafe(this));
		return;
	}

	FArenaMatchFinaleData Data;
	Data.bActive = true;
	Data.WinningTeamId = WinningTeamId;

	const FGameplayTag EliminationTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination"), false);
	const UArenaTeamSubsystem* TeamSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UArenaTeamSubsystem>() : nullptr;
	if (TeamSubsystem != nullptr && EliminationTag.IsValid())
	{
		Data.BlueScore = FMath::Max(0, TeamSubsystem->GetTeamTagStackCount(ArenaMatchFinale::BlueTeamId, EliminationTag));
		Data.RedScore = FMath::Max(0, TeamSubsystem->GetTeamTagStackCount(ArenaMatchFinale::RedTeamId, EliminationTag));
	}
	else
	{
		UE_LOG(LogArena, Warning, TEXT("BeginMatchFinale could not read team scores; showing zero scores"));
	}

	UE_LOG(LogArena, Log, TEXT("Match finale started: WinningTeam=%d Blue=%d Red=%d"), WinningTeamId, Data.BlueScore, Data.RedScore);
	GameState->SetMatchFinaleData(Data);
}

float UArenaGamePhaseAbility_MatchFinale::EnterFinaleStage(EArenaMatchFinaleStage Stage)
{
	const FGameplayTag StageCueTag = ArenaMatchFinale::GetStageCueTag(Stage);
	if (!StageCueTag.IsValid())
	{
		UE_LOG(LogArena, Warning, TEXT("EnterFinaleStage ignored: %s has no stage cue"), *UEnum::GetValueAsString(Stage));
		return 0.0f;
	}

	if (ActiveStageCueTag.IsValid())
	{
		K2_RemoveGameplayCue(ActiveStageCueTag);
	}

	if (AArenaGameState* GameState = Cast<AArenaGameState>(GetOwningActorFromActorInfo()))
	{
		FArenaMatchFinaleData Data = GameState->GetMatchFinaleData();
		Data.StageStartServerTime = GameState->GetServerWorldTimeSeconds();
		Data.TimingStage = Stage;
		GameState->SetMatchFinaleData(Data);
	}

	const float RealSeconds = FMath::Max(0.0f, FinaleTiming.GetStageSeconds(Stage));

	FGameplayCueParameters Parameters;
	Parameters.RawMagnitude = RealSeconds;
	K2_AddGameplayCueWithParams(StageCueTag, Parameters, /*bRemoveOnAbilityEnd=*/ true);
	ActiveStageCueTag = StageCueTag;

	// Ability waits run in dilated game time; scale so the stage lasts its configured real seconds.
	const UWorld* World = GetWorld();
	const AWorldSettings* WorldSettings = World ? World->GetWorldSettings() : nullptr;
	return RealSeconds * (WorldSettings ? WorldSettings->GetEffectiveTimeDilation() : 1.0f);
}

void UArenaGamePhaseAbility_MatchFinale::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AArenaGameState* GameState = ActorInfo ? Cast<AArenaGameState>(ActorInfo->OwnerActor.Get()) : nullptr)
	{
		if (GameState->GetMatchFinaleData().bActive)
		{
			UE_LOG(LogArena, Log, TEXT("Match finale ended in stage %s (cancelled: %d)"), *UEnum::GetValueAsString(GameState->GetMatchFinaleStage()), bWasCancelled ? 1 : 0);
			GameState->SetMatchFinaleData(FArenaMatchFinaleData());
		}

		// The scoring component adds this cue before starting the phase, so it is not in the ability's cue list.
		// Removing it also releases the looping notify's reference to the result screen.
		if (GameState->HasAuthority())
		{
			GameState->GetArenaAbilitySystemComponent()->RemoveGameplayCue(TAG_GameplayCue_MatchDecided_Finale);
		}
	}

	ActiveStageCueTag = FGameplayTag();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
