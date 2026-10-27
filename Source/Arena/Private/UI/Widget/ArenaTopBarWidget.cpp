// Copyright Ludens Studio. All Rights Reserved.

#include "UI/Widget/ArenaTopBarWidget.h"

#include "Components/ActorComponent.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr float SnapshotInterval = 0.05f;

	// Compatibility adapter for the existing Blueprint-owned replicated TDM state.
	// Resolve reflected names rather than loading a Blueprint class into the runtime module.
	bool ReadTDMNumber(const UObject* Source, FName Name, double& Value)
	{
		if (!Source)
		{
			return false;
		}

		const FNumericProperty* Property = FindFProperty<FNumericProperty>(Source->GetClass(), Name);
		if (!Property)
		{
			return false;
		}

		const void* Data = Property->ContainerPtrToValuePtr<void>(Source);
		Value = Property->IsFloatingPoint() ? Property->GetFloatingPointPropertyValue(Data) : double(Property->GetSignedIntPropertyValue(Data));
		return FMath::IsFinite(Value);
	}

	FText NumberText(int32 Value)
	{
		static const FNumberFormattingOptions Options = FNumberFormattingOptions::DefaultNoGrouping();
		return FText::AsNumber(Value, &Options);
	}
} // namespace

void UArenaTopBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SampleClock = 0.0f;
	ApplySnapshotText();
	OnScoreRatiosChanged(GetTeamScoreRatio(0), GetTeamScoreRatio(1), true);
	OnClockStateChanged(ClockState);
}

void UArenaTopBarWidget::NativeDestruct()
{
	ScoringSource.Reset();
	bInitialized = false;
	bHasData = false;
	Super::NativeDestruct();
}

void UArenaTopBarWidget::RefreshSnapshot()
{
	const bool bHadData = bHasData;
	const bool bHadInitializedSnapshot = bInitialized;
	const int32 OldTargetKills = TargetKills;
	const int32 OldSeconds = Seconds;
	const int32 OldKills[2] = {Kills[0], Kills[1]};

	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		ScoringSource.Reset();
		bHasData = false;
		bInitialized = false;
	}
	else
	{
		if (!ScoringSource.IsValid() || ScoringSource->GetOuter() != GameState)
		{
			ScoringSource.Reset();
			bInitialized = false;
			TInlineComponentArray<UActorComponent*> Components(GameState);
			for (UActorComponent* Component : Components)
			{
				if (Component->GetClass()->GetName().StartsWith(TEXT("B_ArenaDeathMatchScoring")))
				{
					ScoringSource = Component;
					break;
				}
			}
		}

		double TargetScore = 0.0;
		double CountdownTime = 0.0;
		UArenaTeamSubsystem* TeamSubsystem = World->GetSubsystem<UArenaTeamSubsystem>();
		const FGameplayTag EliminationTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination"), false);
		bHasData = ReadTDMNumber(ScoringSource.Get(), TEXT("TargetScore"), TargetScore) && ReadTDMNumber(ScoringSource.Get(), TEXT("CountdownTime"), CountdownTime) && TargetScore > 0 && TeamSubsystem && EliminationTag.IsValid();
		if (!bHasData)
		{
			bInitialized = false;
		}
		else
		{
			TargetKills = FMath::Clamp(int32(TargetScore), 1, MAX_int32);
			Seconds = FMath::Max(0, FMath::CeilToInt(CountdownTime));
			// Match GetTeamScore: blue is team 2, red is team 1. Read replicated team tags,
			// not AttackerScore/DefenderScore caches which need not track each elimination.
			const int32 NewKills[2] = {FMath::Max(0, TeamSubsystem->GetTeamTagStackCount(2, EliminationTag)), FMath::Max(0, TeamSubsystem->GetTeamTagStackCount(1, EliminationTag))};
			const bool bWasInitialized = bInitialized;
			for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
			{
				Kills[TeamIndex] = NewKills[TeamIndex];
			}
			bInitialized = true;

			if (bWasInitialized)
			{
				for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
				{
					if (Kills[TeamIndex] > OldKills[TeamIndex])
					{
						OnTeamScored(TeamIndex, Kills[TeamIndex]);
					}
				}
			}
		}
	}

	if (bHadData != bHasData || OldTargetKills != TargetKills || OldSeconds != Seconds || OldKills[0] != Kills[0] || OldKills[1] != Kills[1])
	{
		ApplySnapshotText();
	}

	if (bHadData != bHasData || OldTargetKills != TargetKills || OldKills[0] != Kills[0] || OldKills[1] != Kills[1])
	{
		const bool bInitialize = !bHadInitializedSnapshot || !bHasData || OldTargetKills != TargetKills ||
			Kills[0] < OldKills[0] || Kills[1] < OldKills[1];
		OnScoreRatiosChanged(GetTeamScoreRatio(0), GetTeamScoreRatio(1), bInitialize);
	}

	EArenaTopBarClockState NewState = EArenaTopBarClockState::Syncing;
	if (bHasData)
	{
		NewState = Seconds <= 0 ? EArenaTopBarClockState::Expired
			: Seconds <= CriticalSeconds ? EArenaTopBarClockState::Critical
			: Seconds <= WarningSeconds ? EArenaTopBarClockState::Warning
			: EArenaTopBarClockState::Normal;
	}
	SetClockState(NewState);
}

void UArenaTopBarWidget::ApplySnapshotText()
{
	const FText Placeholder = FText::FromString(TEXT("--"));
	if (BlueKillsText)
	{
		BlueKillsText->SetText(bHasData ? NumberText(Kills[0]) : Placeholder);
	}
	if (RedKillsText)
	{
		RedKillsText->SetText(bHasData ? NumberText(Kills[1]) : Placeholder);
	}

	const FText Goal = bHasData ? NumberText(TargetKills) : Placeholder;
	if (BlueGoalText)
	{
		BlueGoalText->SetText(Goal);
	}
	if (RedGoalText)
	{
		RedGoalText->SetText(Goal);
	}

	if (ClockText)
	{
		ClockText->SetText(FText::FromString(bHasData ? FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60) : FString(TEXT("--:--"))));
	}
}

float UArenaTopBarWidget::GetTeamScoreRatio(int32 TeamIndex) const
{
	if (!bHasData || TargetKills <= 0 || TeamIndex < 0 || TeamIndex >= 2)
	{
		return 0.0f;
	}

	return FMath::Clamp(float(Kills[TeamIndex]) / TargetKills, 0.0f, 1.0f);
}

void UArenaTopBarWidget::SetClockState(EArenaTopBarClockState NewState)
{
	if (ClockState != NewState)
	{
		ClockState = NewState;
		OnClockStateChanged(ClockState);
	}
}

void UArenaTopBarWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);

	SampleClock -= DeltaTime;
	if (SampleClock <= 0)
	{
		RefreshSnapshot();
		SampleClock = SnapshotInterval;
	}
}
