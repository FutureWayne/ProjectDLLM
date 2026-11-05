// Copyright Ludens Studio. All Rights Reserved.

#include "UI/Widget/ArenaMatchFinaleWidget.h"

#include "ArenaLogChannels.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CommonSessionSubsystem.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GameState/ArenaGameState.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Messaging/CommonGameDialog.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "NativeGameplayTags.h"
#include "Player/ArenaPlayerState.h"
#include "PrimaryGameLayout.h"
#include "UITag.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaMatchFinaleWidget)

#define LOCTEXT_NAMESPACE "ArenaMatchFinale"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_ESCAPE_FINALE, "UI.Action.Escape");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_GAME_FINALE, "UI.Layer.Game");

namespace ArenaMatchFinaleWidget
{
	constexpr float SampleInterval = 0.05f;

	FText IntegerText(int32 Value)
	{
		static const FNumberFormattingOptions Options = FNumberFormattingOptions::DefaultNoGrouping();
		return FText::AsNumber(Value, &Options);
	}

	FText PlaceholderText()
	{
		return FText::FromString(TEXT("--"));
	}

	// Distinct from INDEX_NONE ("no value") so the first refresh always writes the placeholder.
	constexpr int32 UnwrittenSeconds = MIN_int32;

	void SetTextIfChanged(UTextBlock* TextBlock, const FText& NewText)
	{
		if (TextBlock != nullptr && !TextBlock->GetText().EqualTo(NewText))
		{
			TextBlock->SetText(NewText);
		}
	}

	/** Independent of the widget so a confirmation answered after the widget is gone still leaves correctly. */
	void LeaveMatchToFrontEnd(ULocalPlayer* LocalPlayer, const TSoftObjectPtr<UWorld>& FrontEndMap)
	{
		if (LocalPlayer == nullptr)
		{
			return;
		}

		if (UGameInstance* GameInstance = LocalPlayer->GetGameInstance())
		{
			if (UCommonSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<UCommonSessionSubsystem>())
			{
				SessionSubsystem->CleanUpSessions();
			}
		}

		if (FrontEndMap.IsNull())
		{
			UE_LOG(LogArena, Error, TEXT("Match finale exit confirmed but no FrontEndMap is configured"));
			return;
		}

		UE_LOG(LogArena, Log, TEXT("Match finale exit confirmed; opening %s"), *FrontEndMap.ToString());
		UGameplayStatics::OpenLevelBySoftObjectPtr(LocalPlayer, FrontEndMap);
	}
}

void UArenaMatchFinaleWidget::SetCueWinningTeam(int32 WinningTeamId)
{
	CueWinningTeamId = WinningTeamId;
	ResolveOutcome();
}

float UArenaMatchFinaleWidget::GetStageProgress() const
{
	const AArenaGameState* GameState = FinaleGameState.Get();
	return GameState ? GameState->GetMatchFinaleStageProgress() : 0.0f;
}

float UArenaMatchFinaleWidget::GetStageRemainingSeconds() const
{
	const AArenaGameState* GameState = FinaleGameState.Get();
	return GameState ? GameState->GetMatchFinaleStageRemainingSeconds() : 0.0f;
}

float UArenaMatchFinaleWidget::GetStageElapsedSeconds() const
{
	const AArenaGameState* GameState = FinaleGameState.Get();
	return GameState ? GameState->GetMatchFinaleStageElapsedSeconds() : 0.0f;
}

void UArenaMatchFinaleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!KillStatTag.IsValid())
	{
		KillStatTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination"), false);
	}

	if (!DeathStatTag.IsValid())
	{
		DeathStatTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Death"), false);
	}

	bWidgetConstructed = true;
	CurrentStage = EArenaMatchFinaleStage::None;
	bResultRevealed = false;
	LastStatsSeconds = ArenaMatchFinaleWidget::UnwrittenSeconds;
	LastNextMatchSeconds = ArenaMatchFinaleWidget::UnwrittenSeconds;
	SampleClock = 0.0f;

	// Without a stage cue yet, fall back to a local lead-in measured from construction.
	const UWorld* World = GetWorld();
	RevealAtRealTime = (World ? World->GetRealTimeSeconds() : 0.0) + ResultRevealDelaySeconds;

	SuppressOtherUI();

	if (UCommonUIActionRouterBase* Router = UCommonUIActionRouterBase::Get(*this))
	{
		InputModeChangedHandle = Router->OnActiveInputModeChanged().AddUObject(this, &ThisClass::HandleInputModeChanged);
	}

	RegisterEscapeBinding();
	BindFinaleGameState();
	ResolveOutcome();
	RefreshTexts();
}

void UArenaMatchFinaleWidget::NativeDestruct()
{
	ReleaseFinaleUI();

	Super::NativeDestruct();
}

void UArenaMatchFinaleWidget::ReleaseFinaleUI()
{
	bWidgetConstructed = false;
	UnbindFinaleGameState();

	if (UCommonUIActionRouterBase* Router = UCommonUIActionRouterBase::Get(*this))
	{
		Router->OnActiveInputModeChanged().Remove(InputModeChangedHandle);
	}

	InputModeChangedHandle.Reset();
	UnregisterEscapeBinding();
	RestoreSuppressedUI();
}

void UArenaMatchFinaleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (!bWidgetConstructed)
	{
		return;
	}

	Super::NativeTick(MyGeometry, InDeltaTime);

	SampleClock -= InDeltaTime;
	if (SampleClock > 0.0f)
	{
		return;
	}
	SampleClock = ArenaMatchFinaleWidget::SampleInterval;

	if (!FinaleGameState.IsValid())
	{
		BindFinaleGameState();
	}

	// Pick up widgets that were added to the viewport after the finale started.
	SuppressOtherUI();

	// The replicated data can arrive after the stage cue, and may override the winner from the MatchDecided cue.
	ResolveOutcome();

	if (!bResultRevealed)
	{
		const UWorld* World = GetWorld();
		if (World != nullptr && World->GetRealTimeSeconds() >= RevealAtRealTime)
		{
			TryRevealResult(false);
		}
	}

	RefreshTexts();
}

void UArenaMatchFinaleWidget::BindFinaleGameState()
{
	if (FinaleGameState.IsValid())
	{
		return;
	}

	const UWorld* World = GetWorld();
	AArenaGameState* GameState = World ? World->GetGameState<AArenaGameState>() : nullptr;
	if (GameState == nullptr)
	{
		return;
	}

	FinaleGameState = GameState;
	StageChangedHandle = GameState->OnMatchFinaleStageChanged.AddUObject(this, &ThisClass::HandleStageChanged);

	if (GameState->GetMatchFinaleStage() != EArenaMatchFinaleStage::None)
	{
		ApplyStage(GameState->GetMatchFinaleStage(), CurrentStage, true);
	}
}

void UArenaMatchFinaleWidget::UnbindFinaleGameState()
{
	if (AArenaGameState* GameState = FinaleGameState.Get())
	{
		GameState->OnMatchFinaleStageChanged.Remove(StageChangedHandle);
	}

	StageChangedHandle.Reset();
	FinaleGameState.Reset();
}

void UArenaMatchFinaleWidget::HandleStageChanged(EArenaMatchFinaleStage NewStage, EArenaMatchFinaleStage OldStage)
{
	// Use the screen's own previous stage: it may have skipped stages the GameState went through.
	ApplyStage(NewStage, CurrentStage, false);
}

void UArenaMatchFinaleWidget::ApplyStage(EArenaMatchFinaleStage NewStage, EArenaMatchFinaleStage OldStage, bool bSnap)
{
	if (NewStage == CurrentStage)
	{
		return;
	}

	CurrentStage = NewStage;
	LastStatsSeconds = ArenaMatchFinaleWidget::UnwrittenSeconds;
	LastNextMatchSeconds = ArenaMatchFinaleWidget::UnwrittenSeconds;

	ResolveOutcome();
	UpdateRevealSchedule();
	RefreshTexts();

	OnFinaleStageChanged(NewStage, OldStage, bSnap);

	if (NewStage == EArenaMatchFinaleStage::None)
	{
		// End immediately even if a viewport or a looping notify still retains the Slate widget this frame.
		ReleaseFinaleUI();
		RemoveFromParent();
		return;
	}

	if (!bResultRevealed && NewStage > EArenaMatchFinaleStage::Result)
	{
		TryRevealResult(true);
	}
}

void UArenaMatchFinaleWidget::ResolveOutcome()
{
	const int32 WinningTeamId = GetWinningTeamId();
	if (WinningTeamId == INDEX_NONE)
	{
		return;
	}

	EArenaMatchOutcome NewOutcome = EArenaMatchOutcome::Draw;
	if (WinningTeamId > 0)
	{
		const AArenaPlayerState* PlayerState = GetOwningPlayerState<AArenaPlayerState>();
		const int32 LocalTeamId = PlayerState ? PlayerState->GetTeamId() : INDEX_NONE;
		if (LocalTeamId == INDEX_NONE)
		{
			// Spectators and unassigned players have no side; show the neutral variant.
			NewOutcome = EArenaMatchOutcome::Draw;
		}
		else
		{
			NewOutcome = LocalTeamId == WinningTeamId ? EArenaMatchOutcome::Victory : EArenaMatchOutcome::Defeat;
		}
	}

	if (!bOutcomeResolved || NewOutcome != Outcome)
	{
		Outcome = NewOutcome;
		bOutcomeResolved = true;
		OnOutcomeResolved(Outcome);
	}
}

void UArenaMatchFinaleWidget::UpdateRevealSchedule()
{
	if (bResultRevealed || CurrentStage != EArenaMatchFinaleStage::Result)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// Measure the lead-in from when the stage began so a screen created after the stage cue still reveals on time.
	const AArenaGameState* GameState = FinaleGameState.Get();
	const float ElapsedSeconds = GameState != nullptr ? GameState->GetMatchFinaleStageElapsedSeconds() : 0.0f;
	RevealAtRealTime = World->GetRealTimeSeconds() + FMath::Max(0.0f, ResultRevealDelaySeconds - ElapsedSeconds);
}

void UArenaMatchFinaleWidget::TryRevealResult(bool bSnap)
{
	if (bResultRevealed || !bOutcomeResolved)
	{
		return;
	}

	bResultRevealed = true;
	OnResultRevealed(Outcome, bSnap);
}

void UArenaMatchFinaleWidget::RefreshTexts()
{
	using namespace ArenaMatchFinaleWidget;

	const AArenaGameState* GameState = FinaleGameState.Get();
	const FArenaMatchFinaleData* FinaleData = GameState != nullptr && GameState->GetMatchFinaleData().bActive ? &GameState->GetMatchFinaleData() : nullptr;

	SetTextIfChanged(BlueScoreText, FinaleData ? IntegerText(FinaleData->BlueScore) : PlaceholderText());
	SetTextIfChanged(RedScoreText, FinaleData ? IntegerText(FinaleData->RedScore) : PlaceholderText());

	const AArenaPlayerState* PlayerState = GetOwningPlayerState<AArenaPlayerState>();
	if (PlayerState != nullptr && KillStatTag.IsValid() && DeathStatTag.IsValid())
	{
		const int32 Kills = FMath::Max(0, PlayerState->GetStatTagStackCount(KillStatTag));
		const int32 Deaths = FMath::Max(0, PlayerState->GetStatTagStackCount(DeathStatTag));

		FNumberFormattingOptions RatioOptions = FNumberFormattingOptions::DefaultNoGrouping();
		RatioOptions.SetMinimumFractionalDigits(2);
		RatioOptions.SetMaximumFractionalDigits(2);
		RatioOptions.SetRoundingMode(ERoundingMode::HalfFromZero);

		SetTextIfChanged(KillsText, IntegerText(Kills));
		SetTextIfChanged(DeathsText, IntegerText(Deaths));
		SetTextIfChanged(KDText, FText::AsNumber(static_cast<double>(Kills) / FMath::Max(1, Deaths), &RatioOptions));
	}
	else
	{
		SetTextIfChanged(KillsText, PlaceholderText());
		SetTextIfChanged(DeathsText, PlaceholderText());
		SetTextIfChanged(KDText, PlaceholderText());
	}

	RefreshCountdowns();
}

void UArenaMatchFinaleWidget::RefreshCountdowns()
{
	using namespace ArenaMatchFinaleWidget;

	const AArenaGameState* GameState = FinaleGameState.Get();
	const int32 RemainingSeconds = GameState != nullptr ? FMath::Max(0, FMath::CeilToInt(GameState->GetMatchFinaleStageRemainingSeconds())) : INDEX_NONE;

	const int32 StatsSeconds = CurrentStage == EArenaMatchFinaleStage::PlayerStats ? RemainingSeconds : INDEX_NONE;
	if (StatsSeconds != LastStatsSeconds)
	{
		LastStatsSeconds = StatsSeconds;
		SetTextIfChanged(StatsCountdownText, StatsSeconds == INDEX_NONE ? PlaceholderText() : IntegerText(StatsSeconds));
	}

	const int32 NextMatchSeconds = CurrentStage == EArenaMatchFinaleStage::NextMatch ? RemainingSeconds : INDEX_NONE;
	if (NextMatchSeconds != LastNextMatchSeconds)
	{
		LastNextMatchSeconds = NextMatchSeconds;
		SetTextIfChanged(NextMatchCountdownText, NextMatchSeconds == INDEX_NONE
			? PlaceholderText()
			: FText::FromString(FString::Printf(TEXT("%02d"), NextMatchSeconds)));
	}
}

void UArenaMatchFinaleWidget::SuppressOtherUI()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (!bUISuppressed)
	{
		bUISuppressed = true;
		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			if (AHUD* HUD = PlayerController->GetHUD())
			{
				SuppressedHUD = HUD;
				bSavedShowHUD = HUD->bShowHUD;
				HUD->bShowHUD = false;
			}
		}
	}

	// Hide the in-game HUD layer but keep menu, game-menu and modal layers so the scoreboard and dialogs stay usable.
	if (UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(GetOwningPlayer()))
	{
		SuppressWidget(RootLayout->GetLayerWidget(TAG_UI_LAYER_GAME_FINALE));
	}

	TArray<UUserWidget*> ViewportWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ViewportWidgets, UUserWidget::StaticClass(), false);
	for (UUserWidget* Widget : ViewportWidgets)
	{
		if (Widget == nullptr || Widget == this || !Widget->IsInViewport())
		{
			continue;
		}

		if (Widget->IsA<UPrimaryGameLayout>() || Widget->IsA<UArenaMatchFinaleWidget>())
		{
			continue;
		}

		SuppressWidget(Widget);
	}
}

void UArenaMatchFinaleWidget::SuppressWidget(UWidget* Widget)
{
	if (Widget == nullptr)
	{
		return;
	}

	const bool bAlreadyTracked = SuppressedWidgets.ContainsByPredicate([Widget](const FSuppressedWidget& Entry)
	{
		return Entry.Widget.Get() == Widget;
	});

	if (!bAlreadyTracked)
	{
		FSuppressedWidget& Entry = SuppressedWidgets.AddDefaulted_GetRef();
		Entry.Widget = Widget;
		Entry.OriginalVisibility = Widget->GetVisibility();
	}

	// Widgets may try to show themselves again; keep them collapsed without overwriting the saved visibility.
	if (Widget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UArenaMatchFinaleWidget::RestoreSuppressedUI()
{
	if (!bUISuppressed)
	{
		return;
	}

	for (const FSuppressedWidget& Entry : SuppressedWidgets)
	{
		if (UWidget* Widget = Entry.Widget.Get())
		{
			Widget->SetVisibility(Entry.OriginalVisibility);
		}
	}
	SuppressedWidgets.Reset();

	if (AHUD* HUD = SuppressedHUD.Get())
	{
		HUD->bShowHUD = bSavedShowHUD;
	}
	SuppressedHUD.Reset();
	bUISuppressed = false;
}

void UArenaMatchFinaleWidget::RegisterEscapeBinding()
{
	if (EscapeBindingHandle.IsValid())
	{
		return;
	}

	// Persistent bindings are routed before the activatable tree, so ESC reaches the finale instead of
	// opening the pause menu from the hidden HUD layout. Displayed persistent actions skip the reachability check.
	const UCommonUIActionRouterBase* Router = UCommonUIActionRouterBase::Get(*this);
	const ECommonInputMode InputMode = Router ? Router->GetActiveInputMode() : ECommonInputMode::All;
	// All is the active router mode that accepts both binding types, not a wildcard on an individual binding.
	FBindUIActionArgs BindArgs(FUIActionTag::ConvertChecked(TAG_UI_ACTION_ESCAPE_FINALE), true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleEscapeAction));
	BindArgs.bIsPersistent = true;
	BindArgs.InputMode = InputMode == ECommonInputMode::All ? ECommonInputMode::Menu : InputMode;
	EscapeBindingHandle = RegisterUIActionBinding(BindArgs);
}

void UArenaMatchFinaleWidget::UnregisterEscapeBinding()
{
	if (EscapeBindingHandle.IsValid())
	{
		// RemoveActionBinding alone only detaches the handle; unregister so the action can be bound again later.
		EscapeBindingHandle.Unregister();
		RemoveActionBinding(EscapeBindingHandle);
	}
	EscapeBindingHandle = FUIActionBindingHandle();
}

void UArenaMatchFinaleWidget::HandleInputModeChanged(ECommonInputMode InputMode)
{
	if (bWidgetConstructed && !bExitConfirmationOpen)
	{
		UnregisterEscapeBinding();
		RegisterEscapeBinding();
	}
}

void UArenaMatchFinaleWidget::HandleEscapeAction()
{
	// Keep the exit action available whenever the finale's input hints are visible.
	if (CurrentStage == EArenaMatchFinaleStage::None || bExitConfirmationOpen)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UCommonMessagingSubsystem* Messaging = LocalPlayer ? LocalPlayer->GetSubsystem<UCommonMessagingSubsystem>() : nullptr;
	if (Messaging == nullptr)
	{
		UE_LOG(LogArena, Warning, TEXT("Match finale cannot show the exit confirmation: no messaging subsystem"));
		return;
	}

	// Let the dialog receive ESC as its own back action while it is open.
	bExitConfirmationOpen = true;
	UnregisterEscapeBinding();

	UCommonGameDialogDescriptor* Descriptor = UCommonGameDialogDescriptor::CreateConfirmationYesNo(
		LOCTEXT("ExitGameHeader", "Exit Game"),
		LOCTEXT("ExitGameBody", "Leave the match and return to the main menu?"));

	TWeakObjectPtr<UArenaMatchFinaleWidget> WeakThis(this);
	TWeakObjectPtr<ULocalPlayer> WeakLocalPlayer(LocalPlayer);
	const TSoftObjectPtr<UWorld> MapToOpen = FrontEndMap;
	Messaging->ShowConfirmation(Descriptor, FCommonMessagingResultDelegate::CreateLambda(
		[WeakThis, WeakLocalPlayer, MapToOpen](ECommonMessagingResult Result)
		{
			if (UArenaMatchFinaleWidget* Widget = WeakThis.Get())
			{
				Widget->HandleExitConfirmationResult(Result);
			}

			if (Result == ECommonMessagingResult::Confirmed)
			{
				ArenaMatchFinaleWidget::LeaveMatchToFrontEnd(WeakLocalPlayer.Get(), MapToOpen);
			}
		}));
}

void UArenaMatchFinaleWidget::HandleExitConfirmationResult(ECommonMessagingResult Result)
{
	bExitConfirmationOpen = false;
	if (Result != ECommonMessagingResult::Confirmed && bWidgetConstructed)
	{
		RegisterEscapeBinding();
	}
}

int32 UArenaMatchFinaleWidget::GetWinningTeamId() const
{
	const AArenaGameState* GameState = FinaleGameState.Get();
	if (GameState != nullptr && GameState->GetMatchFinaleData().bActive)
	{
		return FMath::Max(0, GameState->GetMatchFinaleData().WinningTeamId);
	}

	return CueWinningTeamId == INDEX_NONE ? INDEX_NONE : FMath::Max(0, CueWinningTeamId);
}

int32 UArenaMatchFinaleWidget::GetLocalTeamId() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const AArenaPlayerState* PlayerState = PlayerController != nullptr
		? PlayerController->GetPlayerState<AArenaPlayerState>()
		: nullptr;
	return PlayerState != nullptr ? PlayerState->GetTeamId() : INDEX_NONE;
}

#undef LOCTEXT_NAMESPACE
