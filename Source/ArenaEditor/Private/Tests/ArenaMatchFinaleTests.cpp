// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaMatchFinaleTestTypes.h"

#include "GameplayTagContainer.h"

UArenaMatchFinaleTestAbility::UArenaMatchFinaleTestAbility()
{
	GamePhaseTag = FGameplayTag::RequestGameplayTag(TEXT("GamePhase.PostGame"));
}

#if WITH_DEV_AUTOMATION_TESTS
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/Phases/ArenaGamePhaseAbility.h"
#include "AbilitySystem/Phases/ArenaGamePhaseSubsystem.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CommonGameInstance.h"
#include "CommonLocalPlayer.h"
#include "Components/ActorComponent.h"
#include "Components/DynamicEntryBox.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "GameplayEffectTypes.h"
#include "GameState/ArenaGameState.h"
#include "HAL/IConsoleManager.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/OutputDeviceNull.h"
#include "MovieScene.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "PlayInEditorDataTypes.h"
#include "Player/ArenaPlayerState.h"
#include "PrimaryGameLayout.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Slate/SceneViewport.h"
#include "Teams/ArenaTeamInfo.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "Tests/ArenaEliminationFeedTestTypes.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/Widget/ArenaMatchFinaleWidget.h"
#include "UI/ArenaUIManagerSubsystem.h"
#include "UI/ArenaConfirmationScreen.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Widgets/SViewport.h"

struct FArenaMatchFinaleWidgetTestAccess
{
	static void Tick(UArenaMatchFinaleWidget* Widget)
	{
		Widget->NativeTick(FGeometry(), 0.1f);
	}

	static FUIActionBindingHandle EscapeBinding(const UArenaMatchFinaleWidget* Widget)
	{
		return Widget->EscapeBindingHandle;
	}

	static bool IsReleased(const UArenaMatchFinaleWidget* Widget)
	{
		return !Widget->bWidgetConstructed && !Widget->bUISuppressed && !Widget->FinaleGameState.IsValid()
			&& !Widget->EscapeBindingHandle.IsValid() && !Widget->InputModeChangedHandle.IsValid();
	}
};

namespace ArenaMatchFinaleTests
{
	const TCHAR* FinaleWidgetPath = TEXT("/Game/UI/Hud/GamePhase/W_MatchDecided_Message.W_MatchDecided_Message_C");
	const TCHAR* PostGamePhasePath = TEXT("/Game/Blueprints/Game/Phases/Phase_PostGame.Phase_PostGame_C");
	constexpr int32 RedTeamId = ArenaMatchFinale::RedTeamId;
	constexpr int32 BlueTeamId = ArenaMatchFinale::BlueTeamId;

	const TArray<EArenaMatchFinaleStage>& FullSequence()
	{
		static const TArray<EArenaMatchFinaleStage> Sequence = {
			EArenaMatchFinaleStage::Result,
			EArenaMatchFinaleStage::FinalScore,
			EArenaMatchFinaleStage::PlayerStats,
			EArenaMatchFinaleStage::NextMatch
		};
		return Sequence;
	}

	FString DescribeStages(const TArray<EArenaMatchFinaleStage>& Stages)
	{
		TArray<FString> Names;
		for (const EArenaMatchFinaleStage Stage : Stages)
		{
			Names.Add(UEnum::GetValueAsString(Stage));
		}
		return FString::Join(Names, TEXT(", "));
	}

	FString TextOf(const UTextBlock* TextBlock)
	{
		return TextBlock != nullptr ? TextBlock->GetText().ToString() : FString(TEXT("<missing>"));
	}

	FString NamedTextOf(UUserWidget* Widget, const TCHAR* Name)
	{
		return TextOf(Widget != nullptr ? Cast<UTextBlock>(Widget->GetWidgetFromName(Name)) : nullptr);
	}

	/** Advances a standalone world's clocks by real seconds without engine ticking. */
	void AdvanceRealTime(UWorld* World, double RealSeconds)
	{
		const double WorldSeconds = RealSeconds * World->GetWorldSettings()->GetEffectiveTimeDilation();
		World->TimeSeconds += WorldSeconds;
		World->UnpausedTimeSeconds += WorldSeconds;
		World->RealTimeSeconds += RealSeconds;
	}

	/** Activates the test ability on the GameState's ASC and returns the running instance. */
	UArenaMatchFinaleTestAbility* ActivateTestAbility(AArenaGameState* GameState)
	{
		UArenaAbilitySystemComponent* AbilitySystem = GameState->GetArenaAbilitySystemComponent();
		FGameplayAbilitySpec Spec(UArenaMatchFinaleTestAbility::StaticClass(), 1);
		const FGameplayAbilitySpecHandle Handle = AbilitySystem->GiveAbilityAndActivateOnce(Spec);
		const FGameplayAbilitySpec* FoundSpec = AbilitySystem->FindAbilitySpecFromHandle(Handle);
		return FoundSpec != nullptr ? Cast<UArenaMatchFinaleTestAbility>(FoundSpec->GetPrimaryInstance()) : nullptr;
	}

	/** Standalone game world with a real AArenaGameState, both TDM teams and one local player. */
	struct FStandaloneFixture
	{
		UGameInstance* Instance = nullptr;
		UWorld* World = nullptr;
		AArenaGameState* GameState = nullptr;
		UArenaTeamSubsystem* Teams = nullptr;
		APlayerController* Controller = nullptr;
		TSharedPtr<SViewport> ViewportWidget;
		TSharedPtr<FSceneViewport> SceneViewport;
		FGameplayTag EliminationTag;
		FGameplayTag DeathTag;

		bool Create(FAutomationTestBase& Test, bool bUseCommonUI = false)
		{
			Instance = bUseCommonUI ? NewObject<UArenaMatchFinaleTestGameInstance>(GEngine) : NewObject<UGameInstance>(GEngine);
			Instance->InitializeStandalone();
			World = Instance->GetWorld();
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();

			GameState = World->SpawnActor<AArenaGameState>();
			if (!Test.TestNotNull(TEXT("Spawn AArenaGameState"), GameState))
			{
				return false;
			}
			World->SetGameState(GameState);

			Teams = World->GetSubsystem<UArenaTeamSubsystem>();
			if (!Test.TestNotNull(TEXT("Team subsystem"), Teams))
			{
				return false;
			}
			for (const int32 TeamId : { RedTeamId, BlueTeamId })
			{
				AArenaTeamInfo* Team = World->SpawnActor<AArenaTeamInfo>();
				FindFProperty<FIntProperty>(Team->GetClass(), TEXT("TeamId"))->SetPropertyValue_InContainer(Team, TeamId);
				Teams->RegisterTeamInfo(Team);
			}

			// A viewport client gives the local player a game instance, so local player subsystems (CommonUI action
			// router, CommonGame messaging) exist exactly as in a real client.
			UGameViewportClient* ViewportClient = NewObject<UGameViewportClient>(GEngine);
			ViewportClient->Init(*Instance->GetWorldContext(), Instance, false);
			Instance->GetWorldContext()->GameViewport = ViewportClient;
			if (bUseCommonUI)
			{
				// A null viewport is treated as a closed game window and removes the player on the next engine tick.
				ViewportWidget = SNew(SViewport);
				SceneViewport = FSceneViewport::Create(ViewportClient, ViewportWidget);
				ViewportClient->Viewport = SceneViewport.Get();
			}

			Controller = bUseCommonUI ? World->SpawnActor<AArenaMatchFinaleInputTestController>() : World->SpawnActor<APlayerController>();
			ULocalPlayer* Player = bUseCommonUI ? NewObject<UCommonLocalPlayer>(GEngine) : NewObject<ULocalPlayer>(GEngine);
			Instance->AddLocalPlayer(Player, FPlatformUserId::CreateFromInternalId(0));
			Controller->SetPlayer(Player);
			if (!Test.TestEqual(TEXT("Local player is registered with the game instance"), Player->GetIndexInGameInstance(), 0))
			{
				return false;
			}

			EliminationTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination"));
			DeathTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Death"));
			return true;
		}

		void Destroy()
		{
			if (Instance != nullptr)
			{
				if (FWorldContext* Context = Instance->GetWorldContext())
				{
					if (Context->GameViewport != nullptr && SceneViewport.IsValid())
					{
						Context->GameViewport->Viewport = nullptr;
					}
					SceneViewport.Reset();
					ViewportWidget.Reset();
					Context->GameViewport = nullptr;
				}
				Instance->Shutdown();
			}
			if (World != nullptr)
			{
				World->DestroyWorld(false);
				// Shutdown keeps the engine world context; a leftover Game context is picked up by GetAnyGameWorld and
				// stalls later PIE automation that waits for that world's match to start.
				GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleStagesTest, "Arena.GameFlow.MatchFinale.Stages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMatchFinaleStagesTest::RunTest(const FString& Parameters)
{
	using namespace ArenaMatchFinaleTests;

	FStandaloneFixture Fixture;
	if (!Fixture.Create(*this))
	{
		Fixture.Destroy();
		return false;
	}
	UWorld* World = Fixture.World;
	AArenaGameState* GameState = Fixture.GameState;
	UArenaAbilitySystemComponent* AbilitySystem = GameState->GetArenaAbilitySystemComponent();
	AWorldSettings* WorldSettings = World->GetWorldSettings();

	TestTrue(TEXT("Idle GameState has no finale stage"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::None);
	TestFalse(TEXT("Idle GameState has no finale data"), GameState->GetMatchFinaleData().bActive);
	TestEqual(TEXT("Idle finale has no remaining time"), GameState->GetMatchFinaleStageRemainingSeconds(), 0.0f);
	TestEqual(TEXT("Idle finale reports full progress"), GameState->GetMatchFinaleStageProgress(), 1.0f);

	const FProperty* DataProperty = FindFProperty<FProperty>(AArenaGameState::StaticClass(), TEXT("MatchFinaleData"));
	TestTrue(TEXT("Finale data is a replicated GameState property"), DataProperty != nullptr && DataProperty->HasAnyPropertyFlags(CPF_Net));

	Fixture.Teams->AddTeamTagStack(BlueTeamId, Fixture.EliminationTag, 24);
	Fixture.Teams->AddTeamTagStack(RedTeamId, Fixture.EliminationTag, 18);

	TArray<EArenaMatchFinaleStage> Stages;
	const FDelegateHandle StagesHandle = GameState->OnMatchFinaleStageChanged.AddLambda([&Stages](EArenaMatchFinaleStage NewStage, EArenaMatchFinaleStage)
	{
		Stages.Add(NewStage);
	});

	UArenaMatchFinaleTestAbility* Ability = ActivateTestAbility(GameState);
	if (!TestNotNull(TEXT("Finale ability activates on the GameState"), Ability))
	{
		GameState->OnMatchFinaleStageChanged.Remove(StagesHandle);
		Fixture.Destroy();
		return false;
	}

	Ability->BeginForTest(BlueTeamId);
	{
		const FArenaMatchFinaleData& Data = GameState->GetMatchFinaleData();
		TestTrue(TEXT("Begin publishes the finale data"), Data.bActive);
		TestEqual(TEXT("Winning team is published"), Data.WinningTeamId, BlueTeamId);
		TestEqual(TEXT("Blue score is frozen from team 2"), Data.BlueScore, 24);
		TestEqual(TEXT("Red score is frozen from team 1"), Data.RedScore, 18);
		TestTrue(TEXT("Stages only start with their cue"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::None);
	}

	// Result runs under the finale slow motion.
	WorldSettings->SetTimeDilation(0.12f);
	const float ResultWait = Ability->EnterForTest(EArenaMatchFinaleStage::Result);
	const FGameplayTag ResultCue = ArenaMatchFinale::GetStageCueTag(EArenaMatchFinaleStage::Result);
	const FGameplayTag FinalScoreCue = ArenaMatchFinale::GetStageCueTag(EArenaMatchFinaleStage::FinalScore);
	TestTrue(TEXT("The Result cue sets the stage"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::Result);
	TestTrue(TEXT("The Result cue stays active for late joiners"), AbilitySystem->IsGameplayCueActive(ResultCue));
	TestTrue(TEXT("The wait is scaled by the active dilation"), FMath::IsNearlyEqual(ResultWait, 4.5f * 0.12f, 1e-4f));
	TestTrue(TEXT("Remaining time is reported in real seconds"), FMath::IsNearlyEqual(GameState->GetMatchFinaleStageRemainingSeconds(), 4.5f, 1e-3f));

	Fixture.Teams->AddTeamTagStack(BlueTeamId, Fixture.EliminationTag, 5);
	TestEqual(TEXT("Scores stay frozen after later eliminations"), GameState->GetMatchFinaleData().BlueScore, 24);

	AdvanceRealTime(World, 4.4);
	TestTrue(TEXT("The countdown runs in real time under slow motion"), FMath::IsNearlyEqual(GameState->GetMatchFinaleStageRemainingSeconds(), 0.1f, 1e-3f));

	WorldSettings->SetTimeDilation(1.0f);
	const float FinalScoreWait = Ability->EnterForTest(EArenaMatchFinaleStage::FinalScore);
	TestTrue(TEXT("The next cue advances the stage"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::FinalScore);
	TestFalse(TEXT("The previous stage cue is removed"), AbilitySystem->IsGameplayCueActive(ResultCue));
	TestTrue(TEXT("The current stage cue is active"), AbilitySystem->IsGameplayCueActive(FinalScoreCue));
	TestTrue(TEXT("The wait uses the restored dilation"), FMath::IsNearlyEqual(FinalScoreWait, 2.0f, 1e-4f));
	TestTrue(TEXT("The stage start is published for late joiners"), FMath::IsNearlyEqual(GameState->GetMatchFinaleData().StageStartServerTime, GameState->GetServerWorldTimeSeconds(), 1e-3));

	// Clients receive both an OnActive RPC and a replicated WhileActive, in either order.
	FGameplayCueParameters StaleParameters;
	StaleParameters.RawMagnitude = 99.0f;
	const int32 StageCountBeforeReplay = Stages.Num();
	GameState->HandleGameplayCue(static_cast<UObject*>(GameState), FinalScoreCue, EGameplayCueEvent::WhileActive, StaleParameters);
	GameState->HandleGameplayCue(static_cast<UObject*>(GameState), ResultCue, EGameplayCueEvent::OnActive, StaleParameters);
	GameState->HandleGameplayCue(static_cast<UObject*>(GameState), FinalScoreCue, EGameplayCueEvent::Removed, StaleParameters);
	TestEqual(TEXT("Duplicate, stale and removal cue events do not change the stage"), Stages.Num(), StageCountBeforeReplay);
	TestTrue(TEXT("Duplicate cue events keep the stage duration"), GameState->GetMatchFinaleStageRemainingSeconds() <= 2.0f);

	Ability->EnterForTest(EArenaMatchFinaleStage::PlayerStats);
	AdvanceRealTime(World, 2.5);
	TestTrue(TEXT("PlayerStats progress follows real time"), FMath::IsNearlyEqual(GameState->GetMatchFinaleStageProgress(), 0.5f, 0.01f));
	TestTrue(TEXT("PlayerStats remaining time follows real time"), FMath::IsNearlyEqual(GameState->GetMatchFinaleStageRemainingSeconds(), 2.5f, 0.01f));
	TestTrue(TEXT("PlayerStats elapsed time follows real time"), FMath::IsNearlyEqual(GameState->GetMatchFinaleStageElapsedSeconds(), 2.5f, 0.01f));

	Ability->EnterForTest(EArenaMatchFinaleStage::NextMatch);
	AdvanceRealTime(World, 10.0);
	TestEqual(TEXT("An overrun stage never reports negative time"), GameState->GetMatchFinaleStageRemainingSeconds(), 0.0f);
	TestEqual(TEXT("An overrun stage reports full progress"), GameState->GetMatchFinaleStageProgress(), 1.0f);
	TestEqual(TEXT("Every stage is seen in order"), DescribeStages(Stages), DescribeStages(FullSequence()));

	TestEqual(TEXT("A stage without a cue is rejected"), Ability->EnterForTest(EArenaMatchFinaleStage::None), 0.0f);
	TestTrue(TEXT("A rejected stage keeps the current one"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::NextMatch);

	const FGameplayTag NextMatchCue = ArenaMatchFinale::GetStageCueTag(EArenaMatchFinaleStage::NextMatch);
	Ability->EndForTest();
	TestTrue(TEXT("Ending the ability ends the stage"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::None);
	TestFalse(TEXT("Ending the ability clears the finale data"), GameState->GetMatchFinaleData().bActive);
	TestFalse(TEXT("Ending the ability removes the stage cue"), AbilitySystem->IsGameplayCueActive(NextMatchCue));

	// A draw and zero-length stages must work too.
	Ability = ActivateTestAbility(GameState);
	if (TestNotNull(TEXT("A new finale can start after the previous one ended"), Ability))
	{
		FArenaMatchFinaleTiming ZeroTiming;
		ZeroTiming.ResultSeconds = -3.0f;
		ZeroTiming.FinalScoreSeconds = 0.0f;
		Ability->SetTimingForTest(ZeroTiming);
		Ability->BeginForTest(0);
		TestEqual(TEXT("Draw is stored as team 0"), GameState->GetMatchFinaleData().WinningTeamId, 0);
		TestEqual(TEXT("A new finale freezes the current scores"), GameState->GetMatchFinaleData().BlueScore, 29);
		TestEqual(TEXT("Negative durations are clamped to zero"), Ability->EnterForTest(EArenaMatchFinaleStage::Result), 0.0f);
		TestEqual(TEXT("Zero-length stages report no remaining time"), GameState->GetMatchFinaleStageRemainingSeconds(), 0.0f);
		TestEqual(TEXT("Zero-length stages report full progress"), GameState->GetMatchFinaleStageProgress(), 1.0f);
		Ability->EndForTest();
	}

	// A player who joins mid-stage receives the active cue with the stage already under way.
	FArenaMatchFinaleData LateData;
	LateData.bActive = true;
	LateData.StageStartServerTime = GameState->GetServerWorldTimeSeconds() - 1.5;
	LateData.TimingStage = EArenaMatchFinaleStage::PlayerStats;
	GameState->SetMatchFinaleData(LateData);
	FGameplayCueParameters LateParameters;
	LateParameters.RawMagnitude = 5.0f;
	GameState->HandleGameplayCue(static_cast<UObject*>(GameState), ArenaMatchFinale::GetStageCueTag(EArenaMatchFinaleStage::PlayerStats), EGameplayCueEvent::WhileActive, LateParameters);
	TestTrue(TEXT("A late joiner's countdown starts part way through the stage"), FMath::IsNearlyEqual(GameState->GetMatchFinaleStageRemainingSeconds(), 3.5f, 0.01f));
	GameState->SetMatchFinaleData(FArenaMatchFinaleData());
	TestTrue(TEXT("Clearing the data ends the local stage"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::None);

	GameState->OnMatchFinaleStageChanged.Remove(StagesHandle);
	Fixture.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleConsoleCommandTest, "Arena.GameFlow.MatchFinale.ConsoleCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMatchFinaleConsoleCommandTest::RunTest(const FString& Parameters)
{
	using namespace ArenaMatchFinaleTests;
	FStandaloneFixture Fixture;
	if (!Fixture.Create(*this))
	{
		Fixture.Destroy();
		return false;
	}
	FOutputDeviceNull Output;
	IConsoleManager& Console = IConsoleManager::Get();
	TestNotNull(TEXT("Finale command is registered"), Console.FindConsoleObject(TEXT("Arena.MatchFinale")));
	Console.ProcessUserConsoleInput(TEXT("Arena.MatchFinale"), Output, Fixture.World);
	TestFalse(TEXT("No scoring component means no synthetic finale"), Fixture.GameState->GetMatchFinaleData().bActive);

	UClass* ScoringClass = LoadClass<UActorComponent>(nullptr,
		TEXT("/Game/Blueprints/Game/GameState/B_ArenaDeathMatchScoring.B_ArenaDeathMatchScoring_C"));
	if (!TestNotNull(TEXT("Load the real scoring component"), ScoringClass))
	{
		Fixture.Destroy();
		return false;
	}
	UActorComponent* Scoring = NewObject<UActorComponent>(Fixture.GameState, ScoringClass);
	Fixture.GameState->AddInstanceComponent(Scoring);
	Scoring->RegisterComponent();
	for (const TCHAR* InvalidCommand : { TEXT("Arena.MatchFinale invalid"), TEXT("Arena.MatchFinale 3"), TEXT("Arena.MatchFinale 1 2") })
	{
		Console.ProcessUserConsoleInput(InvalidCommand, Output, Fixture.World);
		TestFalse(TEXT("Invalid arguments do not start a finale"), Fixture.GameState->GetMatchFinaleData().bActive);
	}

	const FGameplayTag ResultCue = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.UserMessage.MatchDecided"));
	for (const int32 Winner : { 0, RedTeamId, BlueTeamId })
	{
		const FString Command = FString::Printf(TEXT("Arena.MatchFinale %d"), Winner);
		TestTrue(TEXT("The registered console command executes"), Console.ProcessUserConsoleInput(*Command, Output, Fixture.World));
		TestTrue(TEXT("Console command starts the production Result stage"), Fixture.GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::Result);
		TestTrue(TEXT("Console command publishes active finale data"), Fixture.GameState->GetMatchFinaleData().bActive);
		TestEqual(TEXT("Command forwards the selected winner to HandleVictory"), Fixture.GameState->GetMatchFinaleData().WinningTeamId, Winner);
		Console.ProcessUserConsoleInput(TEXT("Arena.MatchFinale 2"), Output, Fixture.World);
		TestEqual(TEXT("Repeated commands cannot replace an active winner"), Fixture.GameState->GetMatchFinaleData().WinningTeamId, Winner);
		TestEqual(TEXT("Repeated commands cannot stack the result cue"), Fixture.GameState->GetArenaAbilitySystemComponent()->GetTagCount(ResultCue), 1);
		Fixture.GameState->GetArenaAbilitySystemComponent()->CancelAllAbilities();
		TestFalse(TEXT("A command-triggered finale can be cancelled"), Fixture.GameState->GetMatchFinaleData().bActive);
		TestTrue(TEXT("Cancelling restores the world clock"), FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(Fixture.World), 1.0f));
	}
	Fixture.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleInitialSyncTest, "Arena.GameFlow.MatchFinale.InitialSync",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMatchFinaleInitialSyncTest::RunTest(const FString& Parameters)
{
	using namespace ArenaMatchFinaleTests;
	FStandaloneFixture Fixture;
	if (!Fixture.Create(*this))
	{
		Fixture.Destroy();
		return false;
	}
	AArenaGameState* State = Fixture.GameState;
	State->SetRole(ROLE_SimulatedProxy);
	const FGameplayTag NextMatchCue = ArenaMatchFinale::GetStageCueTag(EArenaMatchFinaleStage::NextMatch);
	FGameplayCueParameters CueParameters;
	CueParameters.RawMagnitude = 3.0f;

	// Inject receive-order boundaries, not a successful calibration: each production callback must do the work.
	auto ReceiveData = [State](const FArenaMatchFinaleData& Data)
	{
		const FArenaMatchFinaleData OldData = State->MatchFinaleData;
		State->MatchFinaleData = Data;
		State->OnRep_MatchFinaleData(OldData);
	};
	auto ReceiveTime = [State](double ServerSeconds)
	{
		State->ReplicatedWorldTimeSecondsDouble = ServerSeconds;
		State->OnRep_ReplicatedWorldTimeSecondsDouble();
	};
	auto ReceiveCue = [State, NextMatchCue, CueParameters]()
	{
		State->HandleGameplayCue(static_cast<UObject*>(State), NextMatchCue, EGameplayCueEvent::WhileActive, CueParameters);
	};
	auto ResetClock = [State]()
	{
		State->NumServerWorldTimeSecondsDeltas = 0;
		State->SumServerWorldTimeSecondsDelta = 0.0;
		State->ServerWorldTimeSecondsDelta = 0.0f;
	};

	for (int32 Order = 0; Order < 3; ++Order)
	{
		ReceiveData(FArenaMatchFinaleData());
		ResetClock();
		const double ServerSeconds = Fixture.World->GetTimeSeconds() + 120.0;
		FArenaMatchFinaleData Data;
		Data.bActive = true;
		Data.TimingStage = EArenaMatchFinaleStage::NextMatch;
		Data.StageStartServerTime = ServerSeconds - 2.0;
		if (Order == 0)
		{
			ReceiveCue();
			ReceiveData(Data);
			TestEqual(TEXT("Cue and data wait for the first server clock sample"), State->GetMatchFinaleStageRemainingSeconds(), 3.0f);
			ReceiveTime(ServerSeconds);
		}
		else if (Order == 1)
		{
			ReceiveTime(ServerSeconds);
			ReceiveCue();
			FArenaMatchFinaleData PreviousStageData = Data;
			PreviousStageData.TimingStage = EArenaMatchFinaleStage::PlayerStats;
			PreviousStageData.StageStartServerTime -= 5.0;
			ReceiveData(PreviousStageData);
			TestEqual(TEXT("A timestamp for another stage cannot calibrate the current cue"), State->GetMatchFinaleStageRemainingSeconds(), 3.0f);
			ReceiveData(Data);
		}
		else
		{
			ReceiveData(Data);
			ReceiveTime(ServerSeconds);
			ReceiveCue();
		}
		TestTrue(FString::Printf(TEXT("Receive order %d resumes the final second instead of restarting"), Order),
			FMath::IsNearlyEqual(State->GetMatchFinaleStageRemainingSeconds(), 1.0f, 0.001f));
		AdvanceRealTime(Fixture.World, 0.25);
		ReceiveTime(ServerSeconds + 10.0);
		ReceiveCue();
		TestTrue(TEXT("Later clock smoothing and duplicate cues cannot rebase the countdown"),
			FMath::IsNearlyEqual(State->GetMatchFinaleStageRemainingSeconds(), 0.75f, 0.001f));
	}

	ReceiveData(FArenaMatchFinaleData());
	ResetClock();
	ReceiveCue();
	FArenaMatchFinaleData PendingData;
	PendingData.bActive = true;
	PendingData.TimingStage = EArenaMatchFinaleStage::NextMatch;
	ReceiveData(PendingData);
	ReceiveData(FArenaMatchFinaleData());
	ReceiveTime(Fixture.World->GetTimeSeconds() + 120.0);
	TestTrue(TEXT("A late clock sample cannot revive a cancelled pending stage"), State->GetMatchFinaleStage() == EArenaMatchFinaleStage::None);
	State->SetRole(ROLE_Authority);
	Fixture.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleWidgetSnapshotTest, "Arena.UI.MatchFinale.Snapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMatchFinaleWidgetSnapshotTest::RunTest(const FString& Parameters)
{
	using namespace ArenaMatchFinaleTests;

	FStandaloneFixture Fixture;
	if (!Fixture.Create(*this))
	{
		Fixture.Destroy();
		return false;
	}
	UWorld* World = Fixture.World;
	AArenaGameState* GameState = Fixture.GameState;
	UClass* WidgetClass = LoadClass<UArenaMatchFinaleWidget>(nullptr, FinaleWidgetPath);
	UArenaMatchFinaleTestAbility* Ability = ActivateTestAbility(GameState);
	if (!TestNotNull(TEXT("Production finale WBP derives from UArenaMatchFinaleWidget"), WidgetClass) || !TestNotNull(TEXT("Finale ability"), Ability))
	{
		Fixture.Destroy();
		return false;
	}

	AArenaPlayerState* PlayerState = World->SpawnActor<AArenaPlayerState>();
	Fixture.Controller->PlayerState = PlayerState;
	PlayerState->SetGenericTeamId(FGenericTeamId(BlueTeamId));
	PlayerState->AddStatTagStack(Fixture.EliminationTag, 24);
	PlayerState->AddStatTagStack(Fixture.DeathTag, 12);
	Fixture.Teams->AddTeamTagStack(BlueTeamId, Fixture.EliminationTag, 30);
	Fixture.Teams->AddTeamTagStack(RedTeamId, Fixture.EliminationTag, 18);

	auto Tick = [](UArenaMatchFinaleWidget* Widget)
	{
		Widget->NativeTick(FGeometry(), 0.1f);
	};

	UArenaMatchFinaleWidget* Widget = CreateWidget<UArenaMatchFinaleWidget>(Fixture.Controller, WidgetClass);
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	TestTrue(TEXT("Taking the Slate widget constructs the screen"), Widget->bWidgetConstructed);
	TestTrue(TEXT("WBP binds every native text"), Widget->BlueScoreText && Widget->RedScoreText && Widget->KillsText && Widget->DeathsText
		&& Widget->KDText && Widget->StatsCountdownText && Widget->NextMatchCountdownText);
	TestTrue(TEXT("WBP configures the stat tags"), Widget->KillStatTag == Fixture.EliminationTag && Widget->DeathStatTag == Fixture.DeathTag);
	TestFalse(TEXT("WBP configures the exit map"), Widget->FrontEndMap.IsNull());
	TestFalse(TEXT("Outcome waits for the cue or the finale data"), Widget->HasResolvedOutcome());
	TestEqual(TEXT("Winner is unknown before any cue"), Widget->GetWinningTeamId(), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Scores show placeholders before the finale"), TextOf(Widget->BlueScoreText), FString(TEXT("--")));
	TestEqual(TEXT("Kills from the local player state"), TextOf(Widget->KillsText), FString(TEXT("24")));
	TestEqual(TEXT("Deaths from the local player state"), TextOf(Widget->DeathsText), FString(TEXT("12")));
	TestEqual(TEXT("K/D uses two decimals"), TextOf(Widget->KDText), FString(TEXT("2.00")));
	TestEqual(TEXT("Stats countdown placeholder outside PlayerStats"), TextOf(Widget->StatsCountdownText), FString(TEXT("--")));
	TestEqual(TEXT("NextMatch countdown placeholder outside NextMatch"), TextOf(Widget->NextMatchCountdownText), FString(TEXT("--")));

	Widget->SetCueWinningTeam(BlueTeamId);
	TestTrue(TEXT("Cue winner resolves Victory before replication"), Widget->HasResolvedOutcome() && Widget->GetOutcome() == EArenaMatchOutcome::Victory);

	Ability->BeginForTest(BlueTeamId);
	Ability->EnterForTest(EArenaMatchFinaleStage::Result);
	TestTrue(TEXT("Screen follows the stage cue into Result"), Widget->GetCurrentStage() == EArenaMatchFinaleStage::Result);
	Widget->HandleEscapeAction();
	TestTrue(TEXT("ESC is available during Result"), Widget->bExitConfirmationOpen);
	Widget->HandleExitConfirmationResult(ECommonMessagingResult::Declined);
	TestEqual(TEXT("Blue frozen score"), TextOf(Widget->BlueScoreText), FString(TEXT("30")));
	TestEqual(TEXT("Red frozen score"), TextOf(Widget->RedScoreText), FString(TEXT("18")));
	AdvanceRealTime(World, Widget->ResultRevealDelaySeconds * 0.5);
	Tick(Widget);
	TestFalse(TEXT("Result stays hidden during the slow-motion lead-in"), Widget->IsResultRevealed());
	AdvanceRealTime(World, Widget->ResultRevealDelaySeconds * 0.5 + 0.1);
	Tick(Widget);
	TestTrue(TEXT("Result is revealed after the lead-in"), Widget->IsResultRevealed());

	Ability->EnterForTest(EArenaMatchFinaleStage::FinalScore);
	TestTrue(TEXT("Screen follows FinalScore"), Widget->GetCurrentStage() == EArenaMatchFinaleStage::FinalScore);
	Widget->HandleEscapeAction();
	TestTrue(TEXT("ESC is available during FinalScore"), Widget->bExitConfirmationOpen);
	Widget->HandleExitConfirmationResult(ECommonMessagingResult::Declined);
	Ability->EnterForTest(EArenaMatchFinaleStage::PlayerStats);
	AdvanceRealTime(World, 1.2);
	Tick(Widget);
	TestTrue(TEXT("Screen follows PlayerStats"), Widget->GetCurrentStage() == EArenaMatchFinaleStage::PlayerStats);
	TestEqual(TEXT("Stats countdown rounds the remaining time up"), TextOf(Widget->StatsCountdownText), FString(TEXT("4")));
	TestEqual(TEXT("NextMatch countdown stays hidden during PlayerStats"), TextOf(Widget->NextMatchCountdownText), FString(TEXT("--")));

	UArenaMatchFinaleWidget* LateWidget = CreateWidget<UArenaMatchFinaleWidget>(Fixture.Controller, WidgetClass);
	TSharedRef<SWidget> LateSlateWidget = LateWidget->TakeWidget();
	TestTrue(TEXT("Late screen snaps to the running stage"), LateWidget->GetCurrentStage() == EArenaMatchFinaleStage::PlayerStats);
	TestTrue(TEXT("Late screen reveals the result immediately"), LateWidget->IsResultRevealed());
	TestTrue(TEXT("Late screen resolves the outcome from the replicated data"), LateWidget->GetOutcome() == EArenaMatchOutcome::Victory);
	TestEqual(TEXT("Late screen fills the frozen score"), TextOf(LateWidget->BlueScoreText), FString(TEXT("30")));

	PlayerState->RemoveStatTagStack(Fixture.DeathTag, 12);
	Tick(Widget);
	TestEqual(TEXT("Zero deaths display"), TextOf(Widget->DeathsText), FString(TEXT("0")));
	TestEqual(TEXT("K/D divides by one when there are no deaths"), TextOf(Widget->KDText), FString(TEXT("24.00")));

	const bool bEscapeRouted = Widget->EscapeBindingHandle.IsValid();
	Widget->HandleEscapeAction();
	TestTrue(TEXT("ESC in PlayerStats requests the exit confirmation"), Widget->bExitConfirmationOpen);
	if (bEscapeRouted)
	{
		TestFalse(TEXT("ESC binding yields to the confirmation dialog"), Widget->EscapeBindingHandle.IsValid());
	}
	Widget->HandleEscapeAction();
	TestTrue(TEXT("A second ESC does not stack confirmations"), Widget->bExitConfirmationOpen);
	Widget->HandleExitConfirmationResult(ECommonMessagingResult::Declined);
	TestFalse(TEXT("Declining closes the confirmation state"), Widget->bExitConfirmationOpen);
	if (bEscapeRouted)
	{
		TestTrue(TEXT("Declining restores the ESC binding"), Widget->EscapeBindingHandle.IsValid());
	}

	Ability->EnterForTest(EArenaMatchFinaleStage::NextMatch);
	AdvanceRealTime(World, 0.4);
	Tick(Widget);
	TestTrue(TEXT("Screen follows NextMatch"), Widget->GetCurrentStage() == EArenaMatchFinaleStage::NextMatch);
	TestEqual(TEXT("NextMatch countdown uses two digits"), TextOf(Widget->NextMatchCountdownText), FString(TEXT("03")));
	TestEqual(TEXT("Stats countdown hides after PlayerStats"), TextOf(Widget->StatsCountdownText), FString(TEXT("--")));
	Widget->HandleEscapeAction();
	TestTrue(TEXT("ESC remains available during NextMatch"), Widget->bExitConfirmationOpen);
	Widget->HandleExitConfirmationResult(ECommonMessagingResult::Declined);
	AdvanceRealTime(World, 3.0);
	Tick(Widget);
	TestEqual(TEXT("An overrun NextMatch countdown stops at zero"), TextOf(Widget->NextMatchCountdownText), FString(TEXT("00")));

	PlayerState->SetGenericTeamId(FGenericTeamId(RedTeamId));
	Tick(Widget);
	TestTrue(TEXT("Losing team resolves Defeat"), Widget->GetOutcome() == EArenaMatchOutcome::Defeat);
	PlayerState->SetGenericTeamId(FGenericTeamId::NoTeam);
	Tick(Widget);
	TestTrue(TEXT("Players without a team see the neutral variant"), Widget->GetOutcome() == EArenaMatchOutcome::Draw);
	PlayerState->SetGenericTeamId(FGenericTeamId(BlueTeamId));

	Ability->EndForTest();
	TestTrue(TEXT("Screen returns to None when the phase ends"), Widget->GetCurrentStage() == EArenaMatchFinaleStage::None);
	Tick(Widget);
	TestEqual(TEXT("Scores return to placeholders when the phase ends"), TextOf(Widget->BlueScoreText), FString(TEXT("--")));
	TestTrue(TEXT("Ending the phase releases the screen even while Slate is retained"), FArenaMatchFinaleWidgetTestAccess::IsReleased(Widget));
	TestTrue(TEXT("Ending the phase releases late screens too"), FArenaMatchFinaleWidgetTestAccess::IsReleased(LateWidget));
	Widget->HandleExitConfirmationResult(ECommonMessagingResult::Declined);
	TestFalse(TEXT("A late declined dialog cannot rebind ESC after the finale ends"), Widget->EscapeBindingHandle.IsValid());

	Widget = CreateWidget<UArenaMatchFinaleWidget>(Fixture.Controller, WidgetClass);
	SlateWidget = Widget->TakeWidget();

	Ability = ActivateTestAbility(GameState);
	if (TestNotNull(TEXT("A draw finale can start"), Ability))
	{
		Ability->BeginForTest(0);
		Tick(Widget);
		TestEqual(TEXT("Replicated draw takes precedence over the cue winner"), Widget->GetWinningTeamId(), 0);
		TestTrue(TEXT("A draw resolves the neutral variant"), Widget->GetOutcome() == EArenaMatchOutcome::Draw);
		Ability->EndForTest();
	}

	Fixture.Controller->PlayerState = nullptr;
	Tick(Widget);
	TestEqual(TEXT("Kills placeholder without a player state"), TextOf(Widget->KillsText), FString(TEXT("--")));
	TestEqual(TEXT("K/D placeholder without a player state"), TextOf(Widget->KDText), FString(TEXT("--")));

	Widget->NativeDestruct();
	LateWidget->NativeDestruct();
	TestFalse(TEXT("Destruct releases the GameState"), Widget->FinaleGameState.IsValid());
	TestFalse(TEXT("Destruct releases the ESC binding"), Widget->EscapeBindingHandle.IsValid());
	Fixture.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleStageEntryTest, "Arena.UI.MatchFinale.StageEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMatchFinaleStageEntryTest::RunTest(const FString& Parameters)
{
	using namespace ArenaMatchFinaleTests;

	FStandaloneFixture Fixture;
	if (!Fixture.Create(*this))
	{
		Fixture.Destroy();
		return false;
	}

	UClass* WidgetClass = LoadClass<UArenaMatchFinaleWidget>(nullptr, FinaleWidgetPath);
	UWidgetBlueprintGeneratedClass* BlueprintClass = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
	UArenaMatchFinaleTestAbility* Ability = ActivateTestAbility(Fixture.GameState);
	if (!TestNotNull(TEXT("Production finale Blueprint"), BlueprintClass) || !TestNotNull(TEXT("Finale ability"), Ability))
	{
		Fixture.Destroy();
		return false;
	}

	auto FindAnimation = [BlueprintClass](FName Name) -> UWidgetAnimation*
	{
		for (UWidgetAnimation* Animation : BlueprintClass->Animations)
		{
			if (Animation != nullptr && Animation->GetMovieScene() != nullptr && Animation->GetMovieScene()->GetFName() == Name)
			{
				return Animation;
			}
		}
		return nullptr;
	};
	UWidgetAnimation* ResultAnimation = FindAnimation(TEXT("Stage_Result"));
	UWidgetAnimation* RevealAnimation = FindAnimation(TEXT("ResultReveal"));
	UWidgetAnimation* ScoreAnimation = FindAnimation(TEXT("Stage_FinalScore"));
	if (!TestNotNull(TEXT("Result entry timeline"), ResultAnimation) || !TestNotNull(TEXT("Result reveal timeline"), RevealAnimation)
		|| !TestNotNull(TEXT("Score timeline"), ScoreAnimation))
	{
		Ability->EndForTest();
		Fixture.Destroy();
		return false;
	}

	AArenaPlayerState* PlayerState = Fixture.World->SpawnActor<AArenaPlayerState>();
	Fixture.Controller->PlayerState = PlayerState;
	PlayerState->SetGenericTeamId(FGenericTeamId(BlueTeamId));
	Ability->BeginForTest(BlueTeamId);
	Ability->EnterForTest(EArenaMatchFinaleStage::Result);
	AdvanceRealTime(Fixture.World, 0.2);

	// The production cue starts before Slate constructs its screen in normal play as well as on late join.
	UArenaMatchFinaleWidget* Widget = CreateWidget<UArenaMatchFinaleWidget>(Fixture.Controller, WidgetClass);
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	Widget->FlushAnimations();
	TestTrue(TEXT("Creating the screen after the cue preserves its entry animation"), Widget->IsAnimationPlaying(ResultAnimation));
	TestTrue(TEXT("Result entry resumes at the elapsed stage time instead of its final frame"),
		FMath::IsNearlyEqual(Widget->GetAnimationCurrentTime(ResultAnimation), 0.2f, 0.02f));
	TestTrue(TEXT("Widget exposes the real stage clock"), FMath::IsNearlyEqual(Widget->GetStageElapsedSeconds(), 0.2f, 0.001f));

	// Simulate a render hitch without ticking UMG; the next ordinary widget tick must catch up.
	Fixture.World->GetWorldSettings()->SetTimeDilation(0.12f);
	AdvanceRealTime(Fixture.World, 0.45);
	FArenaMatchFinaleWidgetTestAccess::Tick(Widget);
	Widget->FlushAnimations();
	TestTrue(TEXT("The entry animation catches up to real time during slow motion"),
		FMath::IsNearlyEqual(Widget->GetAnimationCurrentTime(ResultAnimation), 0.65f, 0.02f));

	const FFloatProperty* RevealDelay = FindFProperty<FFloatProperty>(WidgetClass, TEXT("ResultRevealDelaySeconds"));
	const float DelaySeconds = RevealDelay->GetPropertyValue_InContainer(Widget);
	AdvanceRealTime(Fixture.World, FMath::Max(0.0f, DelaySeconds + 0.25f - Widget->GetStageElapsedSeconds()));
	UArenaMatchFinaleWidget* LateWidget = CreateWidget<UArenaMatchFinaleWidget>(Fixture.Controller, WidgetClass);
	TSharedRef<SWidget> LateSlateWidget = LateWidget->TakeWidget();
	FArenaMatchFinaleWidgetTestAccess::Tick(LateWidget);
	LateWidget->FlushAnimations();
	TestTrue(TEXT("A screen created during the reveal resumes the reveal timeline"),
		FMath::IsNearlyEqual(LateWidget->GetAnimationCurrentTime(RevealAnimation), 0.25f, 0.02f));
	TestFalse(TEXT("The reveal stops the entry timeline before sharing its tracks"), LateWidget->IsAnimationPlaying(ResultAnimation));

	Ability->EnterForTest(EArenaMatchFinaleStage::FinalScore);
	Widget->FlushAnimations();
	LateWidget->FlushAnimations();
	TestFalse(TEXT("Entering FinalScore stops the earlier result animation"), Widget->IsAnimationPlaying(ResultAnimation));
	TestFalse(TEXT("Entering FinalScore stops an active reveal"), LateWidget->IsAnimationPlaying(RevealAnimation));
	TestTrue(TEXT("The score transition starts normally"), Widget->IsAnimationPlaying(ScoreAnimation));
	AdvanceRealTime(Fixture.World, 0.2);
	UArenaMatchFinaleWidget* ScoreWidget = CreateWidget<UArenaMatchFinaleWidget>(Fixture.Controller, WidgetClass);
	TSharedRef<SWidget> ScoreSlateWidget = ScoreWidget->TakeWidget();
	ScoreWidget->FlushAnimations();
	TestTrue(TEXT("Joining FinalScore resumes its transition instead of skipping it"),
		FMath::IsNearlyEqual(ScoreWidget->GetAnimationCurrentTime(ScoreAnimation), 0.2f, 0.02f));
	TestFalse(TEXT("Joining FinalScore cannot restart the result reveal"), ScoreWidget->IsAnimationPlaying(RevealAnimation));

	Ability->EndForTest();
	Widget->RemoveFromParent();
	LateWidget->RemoveFromParent();
	ScoreWidget->RemoveFromParent();
	Fixture.World->GetWorldSettings()->SetTimeDilation(1.0f);
	Fixture.Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleLayersTest, "Arena.UI.MatchFinale.Layers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMatchFinaleLayersTest::RunTest(const FString& Parameters)
{
	using namespace ArenaMatchFinaleTests;
	struct FLayersFixture
	{
		FStandaloneFixture WorldFixture;
		TStrongObjectPtr<UGameInstance> Instance;
		TSharedPtr<SWidget> RootSlate;
		TSharedPtr<SWidget> FinaleSlate;
	};
	const TSharedRef<FLayersFixture> State = MakeShared<FLayersFixture>();
	FStandaloneFixture& Fixture = State->WorldFixture;
	if (!Fixture.Create(*this, true))
	{
		Fixture.Destroy();
		return false;
	}
	State->Instance.Reset(Fixture.Instance);

	APlayerController* Controller = Fixture.Controller;
	Controller->ClientSetHUD(AHUD::StaticClass());
	UPrimaryGameLayout* RootLayout = UPrimaryGameLayout::GetPrimaryGameLayout(Controller);
	UArenaUIManagerSubsystem* UIManager = Fixture.Instance->GetSubsystem<UArenaUIManagerSubsystem>();
	UArenaMatchFinaleTestAbility* Ability = ActivateTestAbility(Fixture.GameState);
	if (!TestNotNull(TEXT("Production UI policy creates the root layout"), RootLayout)
		|| !TestNotNull(TEXT("Production UI manager"), UIManager) || !TestNotNull(TEXT("Finale ability"), Ability))
	{
		Fixture.Destroy();
		return false;
	}

	AHUD* HUD = Controller->GetHUD();
	TestEqual(TEXT("The UI manager uses the fixture world"), UIManager->GetWorld(), Fixture.World);
	TestEqual(TEXT("The UI manager sees the fixture GameState"), UIManager->GetWorld()->GetGameState<AArenaGameState>(), Fixture.GameState);
	State->RootSlate = RootLayout->TakeWidget();
	HUD->bShowHUD = false;
	UIManager->Tick(0.0f);
	TestTrue(TEXT("Outside a finale, hidden HUD still collapses the root"), RootLayout->GetVisibility() == ESlateVisibility::Collapsed);
	Ability->BeginForTest(BlueTeamId);
	UIManager->Tick(0.0f);
	TestTrue(TEXT("Active finale data preserves the root before the stage cue"), RootLayout->IsVisible());
	HUD->bShowHUD = true;

	AArenaPlayerState* PlayerState = Fixture.World->SpawnActor<AArenaPlayerState>();
	Controller->PlayerState = PlayerState;
	PlayerState->SetOwner(Controller);
	UClass* FinaleClass = LoadClass<UArenaMatchFinaleWidget>(nullptr, FinaleWidgetPath);
	UArenaMatchFinaleWidget* Finale = CreateWidget<UArenaMatchFinaleWidget>(Controller, FinaleClass);
	// The isolated viewport has no game window to construct Slate on the next frame.
	State->FinaleSlate = Finale->TakeWidget();
	Finale->AddToPlayerScreen();
	const FGameplayTag GameLayerTag = FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Game"));
	const FGameplayTag GameMenuTag = FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.GameMenu"));
	const FGameplayTag ModalTag = FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Modal"));
	RootLayout->GetLayerWidget(GameMenuTag)->SetTransitionDuration(0.0f);
	RootLayout->GetLayerWidget(ModalTag)->SetTransitionDuration(0.0f);
	UGameViewportSubsystem* Viewport = UGameViewportSubsystem::Get();
	TestTrue(TEXT("Menu root is above the opaque finale in the viewport"), Viewport->GetWidgetSlot(RootLayout).ZOrder > Viewport->GetWidgetSlot(Finale).ZOrder);

	UClass* ScoreboardClass = LoadClass<UCommonActivatableWidget>(nullptr,
		TEXT("/Game/UI/Scoreboard/W_MatchScoreBoard_Elimination.W_MatchScoreBoard_Elimination_C"));
	UCommonUIActionRouterBase* Router = Controller->GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>();
	for (const EArenaMatchFinaleStage Stage : FullSequence())
	{
		const FString StageName = UEnum::GetValueAsString(Stage);
		// CommonUI registers newly constructed widgets and their input bindings on the next core tick.
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, State, Ability, Stage, StageName, Finale, UIManager, HUD, RootLayout, GameLayerTag, GameMenuTag, ScoreboardClass]()
		{
			Ability->EnterForTest(Stage);
			FArenaMatchFinaleWidgetTestAccess::Tick(Finale);
			UIManager->Tick(0.0f);
			TestFalse(StageName + TEXT(": finale hides the native HUD"), HUD->bShowHUD);
			TestTrue(StageName + TEXT(": the root remains visible"), RootLayout->IsVisible());
			TestTrue(StageName + TEXT(": only the gameplay layer is collapsed"),
				RootLayout->GetLayerWidget(GameLayerTag)->GetVisibility() == ESlateVisibility::Collapsed);
			TestNotNull(StageName + TEXT(": production scoreboard opens on GameMenu"),
				RootLayout->PushWidgetToLayerStack(GameMenuTag, ScoreboardClass));
		}));

		for (int32 Attempt = 0; Attempt < 2; ++Attempt)
		{
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, State, StageName, Router, Finale, UIManager, RootLayout, GameMenuTag]()
			{
				FArenaMatchFinaleWidgetTestAccess::Tick(Finale);
				UIManager->Tick(0.0f);
				UCommonActivatableWidget* Scoreboard = RootLayout->GetLayerWidget(GameMenuTag)->GetActiveWidget();
				TestTrue(StageName + TEXT(": scoreboard remains visible over the finale"), Scoreboard && Scoreboard->IsActivated() && Scoreboard->IsVisible()
					&& RootLayout->GetLayerWidget(GameMenuTag)->IsVisible() && RootLayout->IsVisible());
				const FUIActionBindingHandle Binding = FArenaMatchFinaleWidgetTestAccess::EscapeBinding(Finale);
				TestTrue(StageName + TEXT(": ESC binding is registered before input"), Router->GatherActiveBindings().Contains(Binding));
				Router->ProcessInput(EKeys::Escape, IE_Pressed);
				Router->ProcessInput(EKeys::Escape, IE_Released);
			}));
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, State, StageName, Finale, UIManager, RootLayout, GameMenuTag, ModalTag]()
			{
				FArenaMatchFinaleWidgetTestAccess::Tick(Finale);
				UIManager->Tick(0.0f);
				UCommonActivatableWidget* Dialog = RootLayout->GetLayerWidget(ModalTag)->GetActiveWidget();
				if (!TestNotNull(StageName + TEXT(": ESC opens the real exit confirmation"), Dialog))
				{
					return;
				}
				TestTrue(StageName + TEXT(": confirmation is visible above the scoreboard"), Dialog->IsA<UArenaConfirmationScreen>()
					&& Dialog->IsActivated() && Dialog->IsVisible() && RootLayout->GetLayerWidget(ModalTag)->IsVisible() && RootLayout->IsVisible());
				UDynamicEntryBox* Buttons = Cast<UDynamicEntryBox>(Dialog->GetWidgetFromName(TEXT("EntryBox_Buttons")));
				if (!TestNotNull(TEXT("Production dialog button entries"), Buttons) || !TestEqual(TEXT("Confirmation has Yes and No buttons"), Buttons->GetAllEntries().Num(), 2))
				{
					return;
				}
				UUserWidget* DeclineButton = Buttons->GetAllEntries()[1];
				UFunction* Click = DeclineButton->FindFunction(TEXT("HandleButtonClicked"));
				if (!TestNotNull(TEXT("Production button click handler"), Click))
				{
					return;
				}
				DeclineButton->ProcessEvent(Click, nullptr);
				TestFalse(StageName + TEXT(": declining closes the confirmation"), Dialog->IsActivated());
				TestTrue(StageName + TEXT(": declining restores the finale ESC binding"), FArenaMatchFinaleWidgetTestAccess::EscapeBinding(Finale).IsValid());
				UCommonActivatableWidget* Scoreboard = RootLayout->GetLayerWidget(GameMenuTag)->GetActiveWidget();
				TestTrue(StageName + TEXT(": closing the dialog preserves the scoreboard"), Scoreboard && Scoreboard->IsActivated() && Scoreboard->IsVisible());
			}));
		}
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([State, RootLayout, GameMenuTag]()
		{
			if (UCommonActivatableWidget* Scoreboard = RootLayout->GetLayerWidget(GameMenuTag)->GetActiveWidget())
			{
				Scoreboard->DeactivateWidget();
			}
		}));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, State, Ability, Finale, UIManager, HUD, RootLayout]()
	{
		Ability->EndForTest();
		UIManager->Tick(0.0f);
		TestTrue(TEXT("Cancellation releases the input mode listener and ESC binding"), FArenaMatchFinaleWidgetTestAccess::IsReleased(Finale));
		TestTrue(TEXT("Cancellation restores HUD and root visibility"), HUD->bShowHUD && RootLayout->IsVisible());
		HUD->bShowHUD = false;
		UIManager->Tick(0.0f);
		TestTrue(TEXT("Ordinary HUD hiding works again after cancellation"), RootLayout->GetVisibility() == ESlateVisibility::Collapsed);
		HUD->bShowHUD = true;
		State->FinaleSlate.Reset();
		State->RootSlate.Reset();
		State->WorldFixture.Destroy();
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleInputTest, "Arena.GameFlow.MatchFinale.Input",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMatchFinaleInputTest::RunTest(const FString& Parameters)
{
	using namespace ArenaMatchFinaleTests;
	FStandaloneFixture Fixture;
	if (!Fixture.Create(*this))
	{
		Fixture.Destroy();
		return false;
	}

	AArenaMatchFinaleInputTestController* Controller = Fixture.World->SpawnActor<AArenaMatchFinaleInputTestController>();
	Controller->SetPlayer(Fixture.Controller->Player);
	AArenaPlayerState* PlayerState = Fixture.World->SpawnActor<AArenaPlayerState>();
	Controller->PlayerState = PlayerState;
	APawn* Pawn = Fixture.World->SpawnActor<APawn>();
	Controller->Possess(Pawn);
	UArenaAbilitySystemComponent* AbilitySystem = PlayerState->GetArenaAbilitySystemComponent();
	AbilitySystem->InitAbilityActorInfo(PlayerState, Pawn);

	const FGameplayTag FireTag = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Fire"));
	const FGameplayTag JumpTag = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Jump"));
	const FGameplayTag ScoreboardTag = FGameplayTag::RequestGameplayTag(TEXT("InputTag.ShowScoreboard"));
	auto GrantInput = [AbilitySystem](FGameplayTag Tag)
	{
		FGameplayAbilitySpec Spec(UArenaGameplayAbility::StaticClass(), 1);
		Spec.GetDynamicSpecSourceTags().AddTag(Tag);
		return AbilitySystem->GiveAbility(Spec);
	};
	const FGameplayAbilitySpecHandle Fire = GrantInput(FireTag);
	const FGameplayAbilitySpecHandle Jump = GrantInput(JumpTag);
	const FGameplayAbilitySpecHandle Scoreboard = GrantInput(ScoreboardTag);
	UArenaMatchFinaleTestAbility* Ability = ActivateTestAbility(Fixture.GameState);
	if (!TestNotNull(TEXT("Finale ability"), Ability))
	{
		Fixture.Destroy();
		return false;
	}
	Ability->BeginForTest(BlueTeamId);
	Ability->EnterForTest(EArenaMatchFinaleStage::FinalScore);
	TestFalse(TEXT("FinalScore preserves gameplay input"), Fixture.GameState->IsMatchFinaleGameplayInputBlocked());
	Controller->Input_Move(FInputActionValue(FVector2D(1.0f, 1.0f)));
	TestFalse(TEXT("Movement reaches the pawn before PlayerStats"), Pawn->ConsumeMovementInputVector().IsZero());
	Controller->Input_LookMouse(FInputActionValue(FVector2D(1.0f, 1.0f)));
	TestFalse(TEXT("Look reaches the controller before PlayerStats"), Controller->RotationInput.IsZero());
	Controller->RotationInput = FRotator::ZeroRotator;
	Controller->Input_AbilityInputTagPressed(FireTag);
	AbilitySystem->ProcessAbilityInput(0.0f, false);
	TestTrue(TEXT("Fire is held before the transition"), AbilitySystem->IsAbilityInputActive(FireTag));
	Controller->Input_AbilityInputTagPressed(JumpTag);

	for (const EArenaMatchFinaleStage Stage : { EArenaMatchFinaleStage::PlayerStats, EArenaMatchFinaleStage::NextMatch })
	{
		Ability->EnterForTest(Stage);
		TestTrue(TEXT("Post-score stages block gameplay input"), Fixture.GameState->IsMatchFinaleGameplayInputBlocked());
		Controller->Input_Move(FInputActionValue(FVector2D(1.0f, 1.0f)));
		TestTrue(TEXT("Post-score movement does not reach the pawn"), Pawn->ConsumeMovementInputVector().IsZero());
		Controller->Input_LookMouse(FInputActionValue(FVector2D(1.0f, 1.0f)));
		TestTrue(TEXT("Post-score mouse input does not rotate the view"), Controller->RotationInput.IsZero());
		Controller->Input_AbilityInputTagPressed(FireTag);
		Controller->Input_AbilityInputTagPressed(JumpTag);
		Controller->Input_AbilityInputTagPressed(ScoreboardTag);
		AbilitySystem->ProcessAbilityInput(0.0f, false);
		TestFalse(TEXT("Previously held fire is released"), AbilitySystem->FindAbilitySpecFromHandle(Fire)->InputPressed);
		TestFalse(TEXT("Fire cannot remain held behind the results"), AbilitySystem->IsAbilityInputActive(FireTag));
		TestFalse(TEXT("Queued and new gameplay presses do not activate"), AbilitySystem->FindAbilitySpecFromHandle(Jump)->IsActive());
		TestTrue(TEXT("Scoreboard input still activates"), AbilitySystem->IsAbilityInputActive(ScoreboardTag));
		Controller->Input_AbilityInputTagReleased(ScoreboardTag);
		AbilitySystem->ProcessAbilityInput(0.0f, false);
		TestFalse(TEXT("Scoreboard release still reaches the ability"), AbilitySystem->FindAbilitySpecFromHandle(Scoreboard)->InputPressed);
		AbilitySystem->CancelAbilityHandle(Scoreboard);
	}

	Ability->EndForTest();
	TestFalse(TEXT("Ending the phase restores input without a widget"), Fixture.GameState->IsMatchFinaleGameplayInputBlocked());
	AbilitySystem->CancelAbilityHandle(Fire);
	AbilitySystem->ProcessAbilityInput(0.0f, false);
	TestFalse(TEXT("Blocked presses cannot replay after the phase ends"), AbilitySystem->FindAbilitySpecFromHandle(Jump)->IsActive());
	Controller->Input_Move(FInputActionValue(FVector2D(1.0f, 1.0f)));
	TestFalse(TEXT("Movement works again after cancellation"), Pawn->ConsumeMovementInputVector().IsZero());
	Controller->Input_AbilityInputTagPressed(JumpTag);
	AbilitySystem->ProcessAbilityInput(0.0f, false);
	TestTrue(TEXT("Fresh ability input works again after cancellation"), AbilitySystem->FindAbilitySpecFromHandle(Jump)->IsActive());
	Fixture.Destroy();
	return true;
}

namespace ArenaMatchFinaleNetworkTests
{
	using namespace ArenaMatchFinaleTests;

	/** Stage changes observed on one machine, recorded without keeping the world alive. */
	struct FWorldRecord
	{
		TWeakObjectPtr<AArenaGameState> GameState;
		FDelegateHandle Handle;
		TArray<EArenaMatchFinaleStage> Stages;
	};

	struct FFixture
	{
		TArray<FNetDriverDefinition> SavedDrivers = GEngine->NetDriverDefinitions;
		FSoftClassPath SavedGameInstance = GetDefault<UGameMapsSettings>()->GameInstanceClass;
		TMap<TWeakObjectPtr<UWorld>, FWorldRecord> Records;
		TStrongObjectPtr<UArenaMatchFinaleWidget> ClientWidget;
		TWeakObjectPtr<UWorld> ServerWorld;
		TWeakObjectPtr<UWorld> WidgetWorld;
		TArray<TStrongObjectPtr<UUserWidget>> HUDWidgets;
		TArray<TStrongObjectPtr<UArenaMatchFinaleWidget>> ProductionWidgets;
		TArray<FUIActionBindingHandle> ProductionEscapeBindings;

		~FFixture()
		{
			GEngine->NetDriverDefinitions = SavedDrivers;
			GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = SavedGameInstance;
		}
	};

	class FCheckFinaleNetwork : public IAutomationLatentCommand
	{
	public:
		FCheckFinaleNetwork(FAutomationTestBase* InTest, TSharedRef<FFixture> InFixture)
			: Test(InTest), Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			if (GetCurrentRunTime() > 60.0)
			{
				Test->AddError(FString::Printf(TEXT("Match finale network test timed out at step %d"), Step));
				return true;
			}

			UWorld* Server = nullptr;
			TArray<UWorld*> Clients;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType != EWorldType::PIE || World == nullptr || !World->HasBegunPlay())
				{
					continue;
				}
				if (World->GetNetMode() == NM_ListenServer)
				{
					Server = World;
				}
				else if (World->GetNetMode() == NM_Client)
				{
					Clients.Add(World);
				}
			}
			if (Server == nullptr || Clients.Num() != 2)
			{
				return false;
			}
			if (Fixture->ServerWorld.IsValid() && Fixture->ServerWorld.Get() != Server)
			{
				Test->AddError(TEXT("The server world changed; a cancelled phase must not travel"));
				return true;
			}

			AArenaGameState* GameState = Server->GetGameState<AArenaGameState>();
			if (GameState == nullptr || GameState->PlayerArray.Num() != 3)
			{
				return false;
			}

			switch (Step)
			{
			case 0:
				return Setup(Server, Clients, GameState);
			case 1:
				return DriveStages(Server, Clients, GameState);
			case 2:
				return CheckFinaleEnded(Clients);
			case 3:
				return StartPostGamePhase(Server, Clients, GameState);
			case 4:
				return CheckSlowMotionHandoff(Server, Clients, GameState);
			default:
				return CheckPhaseCancelled(Server, Clients);
			}
		}

	private:
		static AArenaGameState* ClientState(UWorld* Client)
		{
			return Client->GetGameState<AArenaGameState>();
		}

		static bool AllClientsAt(const TArray<UWorld*>& Clients, EArenaMatchFinaleStage Stage)
		{
			for (UWorld* Client : Clients)
			{
				const AArenaGameState* State = ClientState(Client);
				if (State == nullptr || State->GetMatchFinaleStage() != Stage)
				{
					return false;
				}
			}
			return true;
		}

		void Record(UWorld* World, AArenaGameState* State)
		{
			FWorldRecord& Entry = Fixture->Records.FindOrAdd(World);
			Entry.GameState = State;
			const TWeakPtr<FFixture> WeakFixture = Fixture;
			const TWeakObjectPtr<UWorld> WeakWorld = World;
			Entry.Handle = State->OnMatchFinaleStageChanged.AddLambda([WeakFixture, WeakWorld](EArenaMatchFinaleStage NewStage, EArenaMatchFinaleStage)
			{
				if (const TSharedPtr<FFixture> Pinned = WeakFixture.Pin())
				{
					Pinned->Records.FindOrAdd(WeakWorld).Stages.Add(NewStage);
				}
			});
		}

		bool Setup(UWorld* Server, const TArray<UWorld*>& Clients, AArenaGameState* GameState)
		{
			for (UWorld* Client : Clients)
			{
				AArenaGameState* State = ClientState(Client);
				APlayerController* Controller = Client->GetFirstPlayerController();
				if (State == nullptr || State->PlayerArray.Num() != 3 || Controller == nullptr || Controller->PlayerState == nullptr || Controller->GetHUD() == nullptr)
				{
					return false;
				}
			}

			const FGameplayTag EliminationTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination"));
			const FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Death"));
			const int32 Teams[] = { BlueTeamId, RedTeamId, BlueTeamId };
			for (int32 Index = 0; Index < GameState->PlayerArray.Num(); ++Index)
			{
				AArenaPlayerState* PlayerState = CastChecked<AArenaPlayerState>(GameState->PlayerArray[Index]);
				PlayerState->SetGenericTeamId(FGenericTeamId(Teams[Index]));
				PlayerState->AddStatTagStack(EliminationTag, 5 + Index);
				PlayerState->AddStatTagStack(DeathTag, 2);
			}

			Fixture->ServerWorld = Server;
			Record(Server, GameState);
			for (UWorld* Client : Clients)
			{
				Record(Client, ClientState(Client));
			}

			Ability = ActivateTestAbility(GameState);
			if (!Test->TestNotNull(TEXT("Finale ability activates on the server GameState"), Ability.Get()))
			{
				return true;
			}

			FArenaMatchFinaleTiming Timing;
			Timing.ResultSeconds = 1.2f;
			Timing.FinalScoreSeconds = 0.8f;
			Timing.PlayerStatsSeconds = 1.6f;
			Timing.NextMatchSeconds = 1.0f;
			Ability->SetTimingForTest(Timing);
			Ability->BeginForTest(BlueTeamId);
			NextStageIndex = 0;
			Step = 1;
			return false;
		}

		bool DriveStages(UWorld* Server, const TArray<UWorld*>& Clients, AArenaGameState* GameState)
		{
			if (NextStageIndex < FullSequence().Num() && Server->GetRealTimeSeconds() >= NextStageRealTime)
			{
				const EArenaMatchFinaleStage Stage = FullSequence()[NextStageIndex++];
				NextStageRealTime = Server->GetRealTimeSeconds() + Ability->EnterForTest(Stage);
				return false;
			}

			if (GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::PlayerStats && !Fixture->ClientWidget.IsValid() && !Fixture->WidgetWorld.IsValid()
				&& AllClientsAt(Clients, EArenaMatchFinaleStage::PlayerStats))
			{
				return CheckLateWidget(Clients[0], GameState);
			}

			// Run past the last stage's window before checking what every client saw.
			if (NextStageIndex < FullSequence().Num() || Server->GetRealTimeSeconds() < NextStageRealTime || !AllClientsAt(Clients, EArenaMatchFinaleStage::NextMatch))
			{
				return false;
			}

			const FWorldRecord& ServerRecord = Fixture->Records.FindChecked(Server);
			Test->TestEqual(TEXT("Server visits every stage once"), DescribeStages(ServerRecord.Stages), DescribeStages(FullSequence()));
			for (UWorld* Client : Clients)
			{
				const FWorldRecord& ClientRecord = Fixture->Records.FindChecked(Client);
				Test->TestEqual(TEXT("Each client's stage cues arrive in order, once each"), DescribeStages(ClientRecord.Stages), DescribeStages(FullSequence()));
				const FArenaMatchFinaleData& Data = ClientState(Client)->GetMatchFinaleData();
				Test->TestTrue(TEXT("Client reads the active finale data"), Data.bActive);
				Test->TestEqual(TEXT("Client winner"), Data.WinningTeamId, BlueTeamId);
				Test->TestEqual(TEXT("Client blue score"), Data.BlueScore, GameState->GetMatchFinaleData().BlueScore);
				Test->TestEqual(TEXT("Client red score"), Data.RedScore, GameState->GetMatchFinaleData().RedScore);
				Test->TestEqual(TEXT("The stage cue's replicated duration reaches the client countdown"), ClientState(Client)->GetMatchFinaleStageRemainingSeconds(), 0.0f, 0.5f);
			}

			if (UArenaMatchFinaleWidget* Widget = Fixture->ClientWidget.Get())
			{
				APlayerController* Controller = Fixture->WidgetWorld->GetFirstPlayerController();
				Test->TestTrue(TEXT("Client screen reaches NextMatch"), Widget->GetCurrentStage() == EArenaMatchFinaleStage::NextMatch);
				Widget->RemoveFromParent();
				Fixture->ClientWidget.Reset();
				Test->TestTrue(TEXT("Removing the screen restores the in-game HUD"), Controller->GetHUD()->bShowHUD);
			}

			Ability->EndForTest();
			Step = 2;
			return false;
		}

		bool CheckLateWidget(UWorld* Client, AArenaGameState* ServerState)
		{
			UClass* WidgetClass = LoadClass<UArenaMatchFinaleWidget>(nullptr, FinaleWidgetPath);
			if (!Test->TestNotNull(TEXT("Load production finale WBP"), WidgetClass))
			{
				return true;
			}

			APlayerController* Controller = Client->GetFirstPlayerController();
			AHUD* HUD = Controller->GetHUD();
			const bool bHUDShownBefore = HUD->bShowHUD;
			UArenaMatchFinaleWidget* Widget = CreateWidget<UArenaMatchFinaleWidget>(Controller, WidgetClass);
			Widget->AddToPlayerScreen(1000);
			Fixture->ClientWidget.Reset(Widget);
			Fixture->WidgetWorld = Client;

			const AArenaGameState* ClientGameState = ClientState(Client);
			Test->TestTrue(TEXT("Client screen joins the stage from the cue"), Widget->GetCurrentStage() == ClientGameState->GetMatchFinaleStage());
			Test->TestTrue(TEXT("Client screen reveals a stage after Result immediately"), Widget->IsResultRevealed());
			Test->TestTrue(TEXT("Client screen hides the in-game HUD"), bHUDShownBefore && !HUD->bShowHUD);

			const AArenaPlayerState* LocalState = Controller->GetPlayerState<AArenaPlayerState>();
			const EArenaMatchOutcome Expected = LocalState->GetTeamId() == BlueTeamId ? EArenaMatchOutcome::Victory : EArenaMatchOutcome::Defeat;
			Test->TestTrue(TEXT("Client outcome follows its replicated team"), Widget->GetOutcome() == Expected);
			const int32 ExpectedKills = LocalState->GetStatTagStackCount(FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination")));
			Test->TestEqual(TEXT("Client kills follow its replicated stats"), NamedTextOf(Widget, TEXT("KillsText")), FString::FromInt(ExpectedKills));
			Test->TestEqual(TEXT("Client score reads the replicated GameState data"),
				NamedTextOf(Widget, TEXT("BlueScoreText")), FString::FromInt(ServerState->GetMatchFinaleData().BlueScore));
			Test->TestTrue(FString::Printf(TEXT("Client countdown follows the stage (client %.3f, server %.3f)"),
				ClientGameState->GetMatchFinaleStageRemainingSeconds(), ServerState->GetMatchFinaleStageRemainingSeconds()),
				FMath::IsNearlyEqual(ClientGameState->GetMatchFinaleStageRemainingSeconds(), ServerState->GetMatchFinaleStageRemainingSeconds(), 0.35f));
			return false;
		}

		bool CheckFinaleEnded(const TArray<UWorld*>& Clients)
		{
			if (!AllClientsAt(Clients, EArenaMatchFinaleStage::None))
			{
				return false;
			}

			for (UWorld* Client : Clients)
			{
				Test->TestFalse(TEXT("Clients see the finale data cleared"), ClientState(Client)->GetMatchFinaleData().bActive);
			}
			Step = 3;
			return false;
		}

		bool StartPostGamePhase(UWorld* Server, const TArray<UWorld*>& Clients, AArenaGameState* GameState)
		{
			TSubclassOf<UArenaGamePhaseAbility> PhaseClass = LoadClass<UArenaGamePhaseAbility>(nullptr, PostGamePhasePath);
			UArenaGamePhaseSubsystem* Phases = Server->GetSubsystem<UArenaGamePhaseSubsystem>();
			if (!Test->TestNotNull(TEXT("Load Phase_PostGame"), PhaseClass.Get()) || !Test->TestNotNull(TEXT("Game phase subsystem"), Phases))
			{
				return true;
			}
			Test->TestTrue(TEXT("Phase_PostGame derives from the finale phase ability"), PhaseClass->IsChildOf(UArenaGamePhaseAbility_MatchFinale::StaticClass()));

			TArray<UWorld*> Worlds = Clients;
			Worlds.Add(Server);
			for (UWorld* World : Worlds)
			{
				UUserWidget* Overlay = CreateWidget<UUserWidget>(World->GetFirstPlayerController(), UArenaMatchFinaleTestHUDWidget::StaticClass());
				Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);
				Overlay->AddToPlayerScreen();
				Fixture->HUDWidgets.Emplace(Overlay);
			}

			// Use the real scoring entry: it adds MatchDecided before starting Phase_PostGame.
			UClass* ScoringClass = LoadClass<UActorComponent>(nullptr,
				TEXT("/Game/Blueprints/Game/GameState/B_ArenaDeathMatchScoring.B_ArenaDeathMatchScoring_C"));
			if (!Test->TestNotNull(TEXT("Load production TDM scoring component"), ScoringClass))
			{
				return true;
			}
			UActorComponent* Scoring = NewObject<UActorComponent>(GameState, ScoringClass);
			GameState->AddInstanceComponent(Scoring);
			Scoring->RegisterComponent();
			FOutputDeviceNull Output;
			IConsoleManager::Get().ProcessUserConsoleInput(TEXT("Arena.MatchFinale 1"), Output, Clients[0]);
			Test->TestFalse(TEXT("A remote client cannot trigger the finale locally"), ClientState(Clients[0])->GetMatchFinaleData().bActive);
			Test->TestFalse(TEXT("A remote console cannot end the server's match"), GameState->GetMatchFinaleData().bActive);
			Test->TestTrue(TEXT("The host console starts the real scoring flow"),
				IConsoleManager::Get().ProcessUserConsoleInput(TEXT("Arena.MatchFinale"), Output, Server));
			Test->TestTrue(TEXT("Phase_PostGame enters Result"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::Result);
			Test->TestTrue(TEXT("Phase_PostGame publishes the finale data"), GameState->GetMatchFinaleData().bActive);
			Test->TestEqual(TEXT("HandleVictory publishes the winning team"), GameState->GetMatchFinaleData().WinningTeamId, BlueTeamId);
			Test->TestTrue(TEXT("Phase_PostGame applies the finale slow motion during Result"),
				FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(Server), 0.12f, 1e-3f));
			Step = 4;
			return false;
		}

		bool CheckSlowMotionHandoff(UWorld* Server, const TArray<UWorld*>& Clients, AArenaGameState* GameState)
		{
			if (GameState->GetMatchFinaleStage() != EArenaMatchFinaleStage::FinalScore || !AllClientsAt(Clients, EArenaMatchFinaleStage::FinalScore))
			{
				return false;
			}

			TArray<UWorld*> Worlds = Clients;
			Worlds.Add(Server);
			for (UWorld* World : Worlds)
			{
				TArray<UUserWidget*> Widgets;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UArenaMatchFinaleWidget::StaticClass(), false);
				Widgets.RemoveAll([](UUserWidget* Widget) { return !Widget->IsInViewport(); });
				if (Widgets.Num() != 1)
				{
					return false;
				}
			}
			for (UWorld* World : Worlds)
			{
				TArray<UUserWidget*> Widgets;
				UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UArenaMatchFinaleWidget::StaticClass(), false);
				for (UUserWidget* Widget : Widgets)
				{
					if (Widget->IsInViewport())
					{
						UArenaMatchFinaleWidget* FinaleWidget = CastChecked<UArenaMatchFinaleWidget>(Widget);
						Fixture->ProductionWidgets.Emplace(FinaleWidget);
						const FUIActionBindingHandle Binding = FArenaMatchFinaleWidgetTestAccess::EscapeBinding(FinaleWidget);
						Test->TestTrue(TEXT("Production screen registers ESC"), Binding.IsValid());
						Fixture->ProductionEscapeBindings.Add(Binding);
					}
				}
				Test->TestFalse(TEXT("Production screen hides the HUD"), World->GetFirstPlayerController()->GetHUD()->bShowHUD);
			}
			for (const TStrongObjectPtr<UUserWidget>& Overlay : Fixture->HUDWidgets)
			{
				Test->TestTrue(TEXT("Production screen suppresses the viewport overlay"), Overlay->GetVisibility() == ESlateVisibility::Collapsed);
			}

			Test->TestTrue(TEXT("Phase_PostGame restores normal time before FinalScore"),
				FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(Server), 1.0f, 1e-3f));
			// The client server-clock estimate lags behind the slow motion; countdowns must not depend on it.
			for (UWorld* Client : Clients)
			{
				const float ClientRemaining = ClientState(Client)->GetMatchFinaleStageRemainingSeconds();
				const float ServerRemaining = GameState->GetMatchFinaleStageRemainingSeconds();
				Test->TestTrue(FString::Printf(TEXT("Client countdown stays in sync after the slow motion (client %.3f, server %.3f)"),
					ClientRemaining, ServerRemaining), FMath::IsNearlyEqual(ClientRemaining, ServerRemaining, 0.35f));
			}

			// Cancelling the phase must end the finale instead of travelling to the next match.
			GameState->GetArenaAbilitySystemComponent()->CancelAllAbilities();
			Test->TestTrue(TEXT("Cancelling Phase_PostGame ends the stage"), GameState->GetMatchFinaleStage() == EArenaMatchFinaleStage::None);
			Test->TestFalse(TEXT("Cancelling Phase_PostGame clears the finale data"), GameState->GetMatchFinaleData().bActive);
			Test->TestTrue(TEXT("Cancelling Phase_PostGame keeps normal time"),
				FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(Server), 1.0f, 1e-3f));
			Step = 5;
			return false;
		}

		bool CheckPhaseCancelled(UWorld* Server, const TArray<UWorld*>& Clients)
		{
			if (!AllClientsAt(Clients, EArenaMatchFinaleStage::None))
			{
				return false;
			}

			const FGameplayTag PostGameTag = FGameplayTag::RequestGameplayTag(TEXT("GamePhase.PostGame"));
			Test->TestFalse(TEXT("Phase_PostGame is no longer active"), Server->GetSubsystem<UArenaGamePhaseSubsystem>()->IsPhaseActive(PostGameTag));
			TArray<UWorld*> Worlds = Clients;
			Worlds.Add(Server);
			const FGameplayTag ResultCue = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.UserMessage.MatchDecided"));
			for (UWorld* World : Worlds)
			{
				Test->TestTrue(TEXT("Cancellation restores the HUD"), World->GetFirstPlayerController()->GetHUD()->bShowHUD);
				Test->TestFalse(TEXT("Cancellation removes the presentation cue"), World->GetGameState<AArenaGameState>()->GetArenaAbilitySystemComponent()->IsGameplayCueActive(ResultCue));
			}
			for (const TStrongObjectPtr<UArenaMatchFinaleWidget>& Widget : Fixture->ProductionWidgets)
			{
				Test->TestFalse(TEXT("Cancellation removes the actual production screen"), Widget->IsInViewport());
				Test->TestTrue(TEXT("A retained result screen releases its bindings and UI state"), FArenaMatchFinaleWidgetTestAccess::IsReleased(Widget.Get()));
			}
			for (const FUIActionBindingHandle& Binding : Fixture->ProductionEscapeBindings)
			{
				Test->TestFalse(TEXT("Cancellation unregisters the original ESC handle"), Binding.IsValid());
			}
			for (const TStrongObjectPtr<UUserWidget>& Overlay : Fixture->HUDWidgets)
			{
				Test->TestTrue(TEXT("Cancellation restores the overlay's original visibility"), Overlay->GetVisibility() == ESlateVisibility::HitTestInvisible);
			}
			return true;
		}

		FAutomationTestBase* Test;
		TSharedRef<FFixture> Fixture;
		TWeakObjectPtr<UArenaMatchFinaleTestAbility> Ability;
		double NextStageRealTime = 0.0;
		int32 NextStageIndex = 0;
		int32 Step = 0;
	};

	/** Adds a real client connection after NextMatch has already been running for two seconds. */
	class FCheckLateJoin : public IAutomationLatentCommand
	{
	public:
		FCheckLateJoin(FAutomationTestBase* InTest, TSharedRef<FFixture> InFixture)
			: Test(InTest), Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			if (GetCurrentRunTime() > 60.0)
			{
				Test->AddError(TEXT("Late-join finale test timed out"));
				return true;
			}

			UWorld* Server = nullptr;
			TArray<UWorld*> Clients;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType == EWorldType::PIE && World != nullptr && World->HasBegunPlay())
				{
					if (World->GetNetMode() == NM_ListenServer)
					{
						Server = World;
					}
					else if (World->GetNetMode() == NM_Client)
					{
						Clients.Add(World);
					}
				}
			}
			AArenaGameState* ServerState = Server ? Server->GetGameState<AArenaGameState>() : nullptr;
			if (ServerState == nullptr || Clients.Num() < 2)
			{
				return false;
			}

			if (!Ability.IsValid())
			{
				if (ServerState->PlayerArray.Num() != 3)
				{
					return false;
				}
				for (UWorld* Client : Clients)
				{
					AArenaGameState* State = Client->GetGameState<AArenaGameState>();
					if (State == nullptr || State->PlayerArray.Num() != 3 || Client->GetFirstPlayerController() == nullptr
						|| Client->GetFirstPlayerController()->PlayerState == nullptr)
					{
						return false;
					}
				}
				for (UWorld* Client : Clients)
				{
					OriginalClients.Add(Client);
				}
				Ability = ActivateTestAbility(ServerState);
				if (!Test->TestNotNull(TEXT("Activate finale before late join"), Ability.Get()))
				{
					return true;
				}
				FArenaMatchFinaleTiming Timing;
				Timing.NextMatchSeconds = 12.0f;
				Ability->SetTimingForTest(Timing);
				Ability->BeginForTest(BlueTeamId);
				Ability->EnterForTest(EArenaMatchFinaleStage::NextMatch);
				JoinAtRealTime = Server->GetRealTimeSeconds() + 2.0;
				return false;
			}

			if (!bJoinRequested)
			{
				if (Server->GetRealTimeSeconds() >= JoinAtRealTime)
				{
					GEditor->RequestLateJoin();
					bJoinRequested = true;
				}
				return false;
			}

			UWorld* LateClient = nullptr;
			for (UWorld* Client : Clients)
			{
				if (!OriginalClients.Contains(Client))
				{
					LateClient = Client;
				}
			}
			AArenaGameState* LateState = LateClient ? LateClient->GetGameState<AArenaGameState>() : nullptr;
			if (Clients.Num() != 3 || ServerState->PlayerArray.Num() != 4 || LateState == nullptr
				|| LateState->GetMatchFinaleStage() != EArenaMatchFinaleStage::NextMatch || !LateState->GetMatchFinaleData().bActive)
			{
				return false;
			}
			if (FirstSampleRealTime < 0.0)
			{
				FirstSampleRealTime = Server->GetRealTimeSeconds() + 0.25;
				return false;
			}
			if (Server->GetRealTimeSeconds() < FirstSampleRealTime)
			{
				return false;
			}

			const float ClientRemaining = LateState->GetMatchFinaleStageRemainingSeconds();
			const float ServerRemaining = ServerState->GetMatchFinaleStageRemainingSeconds();
			Test->TestTrue(FString::Printf(TEXT("A real late joiner resumes the running countdown (client %.3f, server %.3f)"),
				ClientRemaining, ServerRemaining), FMath::IsNearlyEqual(ClientRemaining, ServerRemaining, 0.35f));
			Test->TestTrue(TEXT("A late joiner does not restart the full NextMatch duration"), ClientRemaining < 10.0f);

			if (PreviousRemaining < 0.0f)
			{
				PreviousRemaining = ClientRemaining;
				FirstSampleRealTime = Server->GetRealTimeSeconds() + 0.75;
				return false;
			}
			Test->TestTrue(TEXT("Later server clock samples do not restart the local countdown"), ClientRemaining < PreviousRemaining - 0.5f);
			Ability->EndForTest();
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FFixture> Fixture;
		TWeakObjectPtr<UArenaMatchFinaleTestAbility> Ability;
		TArray<TWeakObjectPtr<UWorld>> OriginalClients;
		double JoinAtRealTime = 0.0;
		double FirstSampleRealTime = -1.0;
		float PreviousRemaining = -1.0f;
		bool bJoinRequested = false;
	};

	class FReleaseWorldReferences : public IAutomationLatentCommand
	{
	public:
		explicit FReleaseWorldReferences(TSharedRef<FFixture> InFixture)
			: Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			for (const TStrongObjectPtr<UUserWidget>& Overlay : Fixture->HUDWidgets)
			{
				Overlay->RemoveFromParent();
			}
			Fixture->HUDWidgets.Reset();
			for (const TStrongObjectPtr<UArenaMatchFinaleWidget>& Widget : Fixture->ProductionWidgets)
			{
				Widget->RemoveFromParent();
			}
			Fixture->ProductionWidgets.Reset();
			if (UArenaMatchFinaleWidget* Widget = Fixture->ClientWidget.Get())
			{
				Widget->RemoveFromParent();
			}
			Fixture->ClientWidget.Reset();
			for (TPair<TWeakObjectPtr<UWorld>, FWorldRecord>& Pair : Fixture->Records)
			{
				if (AArenaGameState* State = Pair.Value.GameState.Get())
				{
					State->OnMatchFinaleStageChanged.Remove(Pair.Value.Handle);
				}
			}
			Fixture->Records.Reset();
			return true;
		}

	private:
		TSharedRef<FFixture> Fixture;
	};

	class FRestoreFixture : public IAutomationLatentCommand
	{
	public:
		explicit FRestoreFixture(TSharedRef<FFixture> InFixture)
			: Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE)
				{
					return false;
				}
			}
			Fixture.Reset();
			return true;
		}

	private:
		TSharedPtr<FFixture> Fixture;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleNetworkTest, "Arena.GameFlow.MatchFinale.Network",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static bool ScheduleFinaleNetwork(FAutomationTestBase* Test, bool bLateJoin)
{
	using namespace ArenaMatchFinaleNetworkTests;
	if (GEditor->PlayWorld)
	{
		Test->AddError(TEXT("Run in an isolated editor without an active PIE session."));
		return false;
	}

	TSharedRef<FFixture> Fixture = MakeShared<FFixture>();
	GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = FSoftClassPath(UGameInstance::StaticClass());
	FNetDriverDefinition Driver;
	Driver.DefName = NAME_GameNetDriver;
	Driver.DriverClassName = TEXT("/Script/OnlineSubsystemUtils.IpNetDriver");
	Driver.DriverClassNameFallback = Driver.DriverClassName;
	GEngine->NetDriverDefinitions.RemoveAll([](const FNetDriverDefinition& Item) { return Item.DefName == NAME_GameNetDriver; });
	GEngine->NetDriverDefinitions.Add(Driver);

	UWorld* EditorWorld = FAutomationEditorCommonUtils::CreateNewMap();
	EditorWorld->GetWorldSettings()->DefaultGameMode = AArenaEliminationFeedTestGameMode::StaticClass();
	EditorWorld->SpawnActor<APlayerStart>(FVector(0, 0, 200), FRotator::ZeroRotator);
	ULevelEditorPlaySettings* Settings = NewObject<ULevelEditorPlaySettings>();
	Settings->SetPlayNetMode(PIE_ListenServer);
	Settings->SetPlayNumberOfClients(3);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetServerPort(17943);
	Settings->SetClientWindowSize(FIntPoint(640, 360));
	Settings->AddToRoot(); // Balanced by FStartPIEForAutomationCommand.
	FRequestPlaySessionParams Request;
	Request.EditorPlaySettings = Settings;
	Request.GameModeOverride = AArenaEliminationFeedTestGameMode::StaticClass();
	Request.bAllowOnlineSubsystem = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Request));
	if (bLateJoin)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FCheckLateJoin(Test, Fixture));
	}
	else
	{
		ADD_LATENT_AUTOMATION_COMMAND(FCheckFinaleNetwork(Test, Fixture));
	}
	ADD_LATENT_AUTOMATION_COMMAND(FReleaseWorldReferences(Fixture));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FRestoreFixture(Fixture));
	return true;
}

bool FArenaMatchFinaleNetworkTest::RunTest(const FString& Parameters)
{
	return ScheduleFinaleNetwork(this, false);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMatchFinaleLateJoinTest, "Arena.GameFlow.MatchFinale.LateJoin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMatchFinaleLateJoinTest::RunTest(const FString& Parameters)
{
	return ScheduleFinaleNetwork(this, true);
}

#endif
