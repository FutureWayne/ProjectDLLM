// Copyright Ludens Studio. All Rights Reserved.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
#include "ArenaLogChannels.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameState/ArenaGameState.h"
#include "GameState/ArenaMatchFinaleTypes.h"
#include "HAL/IConsoleManager.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace ArenaMatchFinaleDebug
{
	void TriggerFinale(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() > 1 || (Args.Num() == 1 && Args[0] != TEXT("0") && Args[0] != TEXT("1") && Args[0] != TEXT("2")))
		{
			UE_LOG(LogArena, Display, TEXT("Usage: Arena.MatchFinale [WinningTeamId=2] (0=draw, 1=red, 2=blue)"));
			return;
		}

		AArenaGameState* GameState = World && World->IsGameWorld() ? World->GetGameState<AArenaGameState>() : nullptr;
		if (GameState == nullptr)
		{
			UE_LOG(LogArena, Display, TEXT("Arena.MatchFinale requires an active Arena match"));
			return;
		}
		if (!GameState->HasAuthority())
		{
			UE_LOG(LogArena, Display, TEXT("Arena.MatchFinale must be run on the listen host or server"));
			return;
		}
		if (GameState->GetMatchFinaleData().bActive)
		{
			UE_LOG(LogArena, Display, TEXT("Arena.MatchFinale ignored: a finale is already active"));
			return;
		}

		UClass* ScoringClass = LoadClass<UActorComponent>(nullptr,
			TEXT("/Game/Blueprints/Game/GameState/B_ArenaDeathMatchScoring.B_ArenaDeathMatchScoring_C"));
		UActorComponent* Scoring = ScoringClass ? GameState->GetComponentByClass(ScoringClass) : nullptr;
		UFunction* HandleVictory = Scoring ? Scoring->FindFunction(TEXT("HandleVictory")) : nullptr;
		FIntProperty* WinningTeamProperty = HandleVictory ? FindFProperty<FIntProperty>(HandleVictory, TEXT("WiningTeam")) : nullptr;
		if (WinningTeamProperty == nullptr || !WinningTeamProperty->HasAnyPropertyFlags(CPF_Parm))
		{
			UE_LOG(LogArena, Display, TEXT("Arena.MatchFinale requires the TDM scoring component with HandleVictory(WiningTeam)"));
			return;
		}

		const int32 WinningTeamId = Args.IsEmpty() ? ArenaMatchFinale::BlueTeamId : FCString::Atoi(*Args[0]);
		FStructOnScope Parameters(HandleVictory);
		WinningTeamProperty->SetPropertyValue_InContainer(Parameters.GetStructMemory(), WinningTeamId);
		UE_LOG(LogArena, Log, TEXT("Arena.MatchFinale triggering the match result: WinningTeam=%d"), WinningTeamId);

		// Use the scoring Blueprint's normal entry so the widget cue, winner and post-game phase stay consistent.
		Scoring->ProcessEvent(HandleVictory, Parameters.GetStructMemory());
	}

	FAutoConsoleCommandWithWorldAndArgs TriggerFinaleCommand(
		TEXT("Arena.MatchFinale"),
		TEXT("Trigger the full TDM match finale on the host/server. Arena.MatchFinale [WinningTeamId=2]: 0=draw, 1=red, 2=blue. Starts the normal next-match countdown."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TriggerFinale),
		ECVF_Cheat);
}
#endif
