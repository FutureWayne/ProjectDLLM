// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArenaTopBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

UENUM(BlueprintType)
enum class EArenaTopBarClockState : uint8
{
	Syncing,
	Normal,
	Warning,
	Critical,
	Expired
};

/**
 * TDM top bar. Reads the existing replicated scoring component and pushes values into
 * the bound widgets; never owns match state. WBP_ArenaTopBar owns all fonts, colors,
 * layout, styles and effects.
 */
UCLASS(Abstract, Blueprintable)
class ARENA_API UArenaTopBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual void NativeDestruct() override;

	/** Fired when a team's kill count increases after the initial snapshot. TeamIndex: 0 = blue, 1 = red. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|TopBar|Presentation")
	void OnTeamScored(int32 TeamIndex, int32 NewKills);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|TopBar|Presentation")
	void OnClockStateChanged(EArenaTopBarClockState NewState);

	UFUNCTION(BlueprintPure, Category = "Arena|TopBar")
	EArenaTopBarClockState GetClockState() const { return ClockState; }

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BlueKillsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RedKillsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BlueGoalText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RedGoalText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ClockText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BlueScoreBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> RedScoreBar;

	/** Remaining seconds at or below which the clock enters the warning state. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|TopBar", meta = (ClampMin = 0))
	int32 WarningSeconds = 30;

	/** Remaining seconds at or below which the clock enters the critical state. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|TopBar", meta = (ClampMin = 0))
	int32 CriticalSeconds = 10;

	/** Interpolation speed of the score bars towards the current kill ratio. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|TopBar", meta = (ClampMin = 0))
	float ProgressInterpSpeed = 7.0f;

private:
	friend class FArenaTopBarSnapshotTest;

	void RefreshSnapshot();
	void ApplySnapshotText();
	void ApplyProgress();
	void SetClockState(EArenaTopBarClockState NewState);

	TWeakObjectPtr<UObject> ScoringSource;
	int32 Kills[2] = {0, 0};
	int32 TargetKills = 0;
	int32 Seconds = 0;
	float Progress[2] = {0.0f, 0.0f};
	float SampleClock = 0.0f;
	EArenaTopBarClockState ClockState = EArenaTopBarClockState::Syncing;
	bool bInitialized = false;
	bool bHasData = false;
};
