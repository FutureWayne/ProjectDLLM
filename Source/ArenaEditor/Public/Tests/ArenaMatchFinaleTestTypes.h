// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "AbilitySystem/Phases/ArenaGamePhaseAbility_MatchFinale.h"
#include "Blueprint/UserWidget.h"
#include "CommonGameInstance.h"
#include "Player/ArenaPlayerController.h"
#include "ArenaMatchFinaleTestTypes.generated.h"

/** Concrete CommonGame host for isolated tests that use the production UI policy and messaging subsystem. */
UCLASS(Transient)
class UArenaMatchFinaleTestGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()
};

/** The native half of the post-match phase, driven directly so tests choose when each stage starts. */
UCLASS(Transient)
class UArenaMatchFinaleTestAbility : public UArenaGamePhaseAbility_MatchFinale
{
	GENERATED_BODY()

public:
	UArenaMatchFinaleTestAbility();

	void BeginForTest(int32 WinningTeamId) { BeginMatchFinale(WinningTeamId); }
	float EnterForTest(EArenaMatchFinaleStage Stage) { return EnterFinaleStage(Stage); }
	void SetTimingForTest(const FArenaMatchFinaleTiming& Timing) { FinaleTiming = Timing; }
	void EndForTest() { K2_EndAbility(); }
};

/** Viewport HUD stand-in used to observe the finale's visibility restoration. */
UCLASS(Transient)
class UArenaMatchFinaleTestHUDWidget : public UUserWidget
{
	GENERATED_BODY()
};

/** Production input handlers without the experience's input asset setup. */
UCLASS(Transient)
class AArenaMatchFinaleInputTestController : public AArenaPlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override { ACommonPlayerController::BeginPlay(); }
	virtual void SetupInputComponent() override { ACommonPlayerController::SetupInputComponent(); }
};
