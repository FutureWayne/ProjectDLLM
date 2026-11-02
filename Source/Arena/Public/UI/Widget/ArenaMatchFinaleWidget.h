// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "GameState/ArenaMatchFinaleTypes.h"
#include "Input/UIActionBindingHandle.h"
#include "ArenaMatchFinaleWidget.generated.h"

class AArenaGameState;
class AHUD;
class UTextBlock;
class UWorld;
enum class ECommonMessagingResult : uint8;

/** Finale result as seen by the owning local player. */
UENUM(BlueprintType)
enum class EArenaMatchOutcome : uint8
{
	Victory,
	Defeat,
	Draw
};

/**
 * Post-match finale screen. Follows the stage AArenaGameState receives from the post-match phase's GameplayCues,
 * resolves the local outcome, pushes the replicated scores, personal stats and countdowns into bound text widgets,
 * keeps the rest of the in-game HUD hidden while it exists, and handles the PlayerStats exit request. The scoreboard
 * keeps using the existing GA_ShowScoreBoard input path; this widget only leaves menu layers visible so it can appear.
 *
 * The Widget Blueprint owns every layout, color, font, animation and sound decision and reacts to
 * OnFinaleStageChanged / OnOutcomeResolved / OnResultRevealed.
 */
UCLASS(Abstract, Blueprintable)
class ARENA_API UArenaMatchFinaleWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Supplies the winner carried by the MatchDecided GameplayCue (0 = draw, 1 = red, 2 = blue).
	 * Used until the replicated finale data arrives, which then takes precedence.
	 */
	UFUNCTION(BlueprintCallable, Category = "Arena|Finale")
	void SetCueWinningTeam(int32 WinningTeamId);

	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	EArenaMatchFinaleStage GetCurrentStage() const { return CurrentStage; }

	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	EArenaMatchOutcome GetOutcome() const { return Outcome; }

	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	bool HasResolvedOutcome() const { return bOutcomeResolved; }

	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	bool IsResultRevealed() const { return bResultRevealed; }

	/** Winner known to this screen: 0 = draw, 1 = red, 2 = blue, -1 = not known yet. */
	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	int32 GetWinningTeamId() const;

	/** Elapsed fraction of the current stage in [0, 1]. */
	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	float GetStageProgress() const;

	/** Real seconds left in the current stage. */
	UFUNCTION(BlueprintPure, Category = "Arena|Finale")
	float GetStageRemainingSeconds() const;

protected:
	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~End of UUserWidget interface

	/**
	 * Raised for every stage the screen enters. bSnap is true when the screen joins an already running stage
	 * (late construction or late replication) and should jump to that stage's final pose without transitions.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Finale|Presentation")
	void OnFinaleStageChanged(EArenaMatchFinaleStage NewStage, EArenaMatchFinaleStage OldStage, bool bSnap);

	/** Raised when the local outcome becomes known or changes; the WBP selects the Victory / Defeat / Draw variant. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Finale|Presentation")
	void OnOutcomeResolved(EArenaMatchOutcome NewOutcome);

	/** Raised once after the slow-motion lead-in. bSnap is true when the lead-in had already passed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Finale|Presentation")
	void OnResultRevealed(EArenaMatchOutcome RevealedOutcome, bool bSnap);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BlueScoreText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RedScoreText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeathsText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KDText;

	/** Whole seconds left in PlayerStats, i.e. until the NextMatch countdown begins. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatsCountdownText;

	/** Two-digit seconds left in NextMatch. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextMatchCountdownText;

	/** Real seconds after the Result stage starts before the result is revealed; matches the slow-motion lead-in. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Finale", meta = (ClampMin = "0.0", Units = "s"))
	float ResultRevealDelaySeconds = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Finale", meta = (Categories = "Score"))
	FGameplayTag KillStatTag;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Finale", meta = (Categories = "Score"))
	FGameplayTag DeathStatTag;

	/** Map opened after the player confirms leaving from the PlayerStats stage. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Finale")
	TSoftObjectPtr<UWorld> FrontEndMap;

private:
	friend class FArenaMatchFinaleWidgetSnapshotTest;
	friend struct FArenaMatchFinaleWidgetTestAccess;

	void ReleaseFinaleUI();

	void BindFinaleGameState();
	void UnbindFinaleGameState();
	void HandleStageChanged(EArenaMatchFinaleStage NewStage, EArenaMatchFinaleStage OldStage);
	void ApplyStage(EArenaMatchFinaleStage NewStage, EArenaMatchFinaleStage OldStage, bool bSnap);
	void ResolveOutcome();
	void UpdateRevealSchedule();
	void TryRevealResult(bool bSnap);
	void RefreshTexts();
	void RefreshCountdowns();

	void SuppressOtherUI();
	void RestoreSuppressedUI();
	void SuppressWidget(UWidget* Widget);

	void RegisterEscapeBinding();
	void UnregisterEscapeBinding();
	void HandleEscapeAction();
	void HandleExitConfirmationResult(ECommonMessagingResult Result);

	struct FSuppressedWidget
	{
		TWeakObjectPtr<UWidget> Widget;
		ESlateVisibility OriginalVisibility = ESlateVisibility::Visible;
	};

	TWeakObjectPtr<AArenaGameState> FinaleGameState;
	FDelegateHandle StageChangedHandle;
	FUIActionBindingHandle EscapeBindingHandle;

	TArray<FSuppressedWidget> SuppressedWidgets;
	TWeakObjectPtr<AHUD> SuppressedHUD;
	bool bSavedShowHUD = true;
	bool bUISuppressed = false;

	EArenaMatchFinaleStage CurrentStage = EArenaMatchFinaleStage::None;
	EArenaMatchOutcome Outcome = EArenaMatchOutcome::Draw;
	int32 CueWinningTeamId = INDEX_NONE;
	double RevealAtRealTime = 0.0;
	float SampleClock = 0.0f;
	int32 LastStatsSeconds = INDEX_NONE;
	int32 LastNextMatchSeconds = INDEX_NONE;
	bool bOutcomeResolved = false;
	bool bResultRevealed = false;
	bool bExitConfirmationOpen = false;
	bool bWidgetConstructed = false;
};
