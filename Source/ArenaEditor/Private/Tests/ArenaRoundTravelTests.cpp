// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaRoundTravelTestGameMode.h"

AArenaRoundTravelTestGameMode::AArenaRoundTravelTestGameMode()
{
	bStartPlayersAsSpectators = true;
}

void AArenaRoundTravelTestGameMode::RestartPlayer(AController* NewPlayer)
{
	// Match start can request a pawn even for waiting players. This fixture tests travel and UI only.
}

#if WITH_DEV_AUTOMATION_TESTS
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaGameplayAbility.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/PackageMapClient.h"
#include "Engine/World.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameMapsSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "IpConnection.h"
#include "IpNetDriver.h"
#include "UObject/StrongObjectPtr.h"
#include "Player/ArenaPlayerState.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "System/ArenaSystemStatics.h"
#include "Tests/AutomationEditorCommon.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/AsyncAction_ListenForGameplayMessage.h"
#include "HAL/PlatformMemory.h"
#include "UI/Widget/ArenaScoreboardStatWidget.h"
#include "UObject/StructOnScope.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace
{
	class FArenaClientGuidCache : public FNetGUIDCache
	{
	public:
		explicit FArenaClientGuidCache(UNetDriver* Driver)
			: FNetGUIDCache(Driver)
		{
		}

		virtual bool IsNetGUIDAuthority() const override
		{
			return false;
		}
	};

	int32 CountScoreMessageActions()
	{
		int32 Count = 0;
		for (TObjectIterator<UAsyncAction_ListenForGameplayMessage> It; It; ++It)
		{
			if (IsValid(*It) && !It->HasAnyFlags(RF_ClassDefaultObject))
			{
				++Count;
			}
		}
		return Count;
	}

	bool CheckScoreboardLifecycle(FAutomationTestBase* Test, UWorld* World, int32 Round)
	{
		CollectGarbage(RF_NoFlags);
		const int32 InitialActions = CountScoreMessageActions();
		APlayerController* Controller = World->GetFirstPlayerController();
		AArenaPlayerState* PlayerState = Controller->GetPlayerState<AArenaPlayerState>();
		UClass* RowClass = LoadClass<UUserWidget>(nullptr,
			TEXT("/Game/UI/Scoreboard/W_SB_PlayerRow_TDM.W_SB_PlayerRow_TDM_C"));
		if (!Test->TestNotNull(TEXT("Load the real TDM scoreboard row"), RowClass) || !PlayerState)
		{
			return false;
		}
		TWeakObjectPtr<UUserWidget> ReleasedRow;
		{
			TStrongObjectPtr<UUserWidget> Row(CreateWidget<UUserWidget>(Controller, RowClass));
			if (!Test->TestNotNull(TEXT("Create production scoreboard row"), Row.Get()))
			{
				return false;
			}
			ReleasedRow = Row.Get();
			UFunction* SetPlayer = Row->FindFunction(TEXT("SetPlayerState"));
			if (!Test->TestNotNull(TEXT("Use real scoreboard player assignment"), SetPlayer))
			{
				return false;
			}
			FStructOnScope Params(SetPlayer);
			for (TFieldIterator<FObjectPropertyBase> It(SetPlayer); It; ++It)
			{
				if (It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					It->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), PlayerState);
				}
			}
			Row->ProcessEvent(SetPlayer, Params.GetStructMemory());
			Test->TestEqual(TEXT("Assigning data before display creates no listener"), CountScoreMessageActions(), InitialActions);
			TSharedPtr<SWidget> Slate = Row->TakeWidget();
			TArray<UArenaScoreboardStatWidget*> Stats;
			Row->WidgetTree->ForEachWidget([&Stats](UWidget* Widget)
			{
				if (UArenaScoreboardStatWidget* Stat = Cast<UArenaScoreboardStatWidget>(Widget))
				{
					Stats.Add(Stat);
				}
			});
			if (!Test->TestTrue(TEXT("Real row contains migrated stat widgets"), Stats.Num() > 0))
			{
				return false;
			}
			const FObjectPropertyBase* ListenerProperty = FindFProperty<FObjectPropertyBase>(
				UArenaScoreboardStatWidget::StaticClass(), TEXT("ScoreListener"));
			TArray<TWeakObjectPtr<UObject>> OriginalListeners;
			for (UArenaScoreboardStatWidget* Stat : Stats)
			{
				UAsyncAction_ListenForGameplayMessage* Listener = Cast<UAsyncAction_ListenForGameplayMessage>(
					ListenerProperty->GetObjectPropertyValue_InContainer(Stat));
				Test->TestNotNull(TEXT("Displayed stat owns one listener"), Listener);
				Test->TestTrue(TEXT("Displayed listener is registered and bound to its stat"),
					Listener && Listener->IsActive() && Listener->OnMessageReceived.Contains(Stat, TEXT("HandleScoreMessage")));
				OriginalListeners.Add(Listener);
			}
			const int32 ActionsAfterAssignment = CountScoreMessageActions();
			for (int32 Refresh = 0; Refresh < 500; ++Refresh)
			{
				Row->ProcessEvent(SetPlayer, Params.GetStructMemory());
			}
			Test->TestEqual(TEXT("500 production refreshes create no extra async actions"),
				CountScoreMessageActions(), ActionsAfterAssignment);
			for (int32 Index = 0; Index < Stats.Num(); ++Index)
			{
				Test->TestTrue(TEXT("Repeated assignment reuses exactly the same listener"),
					ListenerProperty->GetObjectPropertyValue_InContainer(Stats[Index]) == OriginalListeners[Index].Get());
			}

			Slate.Reset();
			Row->ReleaseSlateResources(true);
			for (UArenaScoreboardStatWidget* Stat : Stats)
			{
				Test->TestNull(TEXT("Closing row immediately releases its listeners"),
					ListenerProperty->GetObjectPropertyValue_InContainer(Stat));
			}
			for (const TWeakObjectPtr<UObject>& Original : OriginalListeners)
			{
				if (UAsyncAction_ListenForGameplayMessage* Listener = Cast<UAsyncAction_ListenForGameplayMessage>(Original.Get()))
				{
					Test->TestFalse(TEXT("Closed listener unregisters from the game instance"), Listener->IsActive());
					Test->TestFalse(TEXT("Closed listener releases widget delegates"), Listener->OnMessageReceived.IsBound());
				}
			}
			// Pooling can reconstruct the same widgets without calling SetPlayerState again.
			Slate = Row->TakeWidget();
			for (UArenaScoreboardStatWidget* Stat : Stats)
			{
				Test->TestNotNull(TEXT("Reopening pooled row restores one listener"),
					ListenerProperty->GetObjectPropertyValue_InContainer(Stat));
			}
			Slate.Reset();
			Row->ReleaseSlateResources(true);
		}
		CollectGarbage(RF_NoFlags);
		Test->TestFalse(TEXT("Discarded scoreboard row is collectible"), ReleasedRow.IsValid());
		Test->TestEqual(TEXT("Listener objects return to baseline after collection"), CountScoreMessageActions(), InitialActions);
		Test->AddInfo(FString::Printf(TEXT("Scoreboard round %d: 500 refreshes, async actions %d -> %d, process memory %.1f MiB"),
			Round, InitialActions, CountScoreMessageActions(), FPlatformMemory::GetStats().UsedPhysical / (1024.0 * 1024.0)));
		return true;
	}

	struct FArenaRoundTravelFixture
	{
		TArray<FNetDriverDefinition> SavedDrivers = GEngine->NetDriverDefinitions;
		TWeakObjectPtr<UArenaAbilitySystemComponent> RetainedASC;
		FGameplayAbilitySpecHandle RetainedAbility;
		FSoftClassPath SavedGameInstance = GetDefault<UGameMapsSettings>()->GameInstanceClass;
		IConsoleVariable* SeamlessPIE = IConsoleManager::Get().FindConsoleVariable(TEXT("net.AllowPIESeamlessTravel"));
		int32 SavedSeamlessPIE = SeamlessPIE->GetInt();
		TArray<TWeakObjectPtr<UNetConnection>> Connections;
		TArray<TWeakObjectPtr<UNetDriver>> Drivers;
		TWeakObjectPtr<UWorld> PreviousServer;
		TArray<TStrongObjectPtr<UArenaScoreboardStatWidget>> OpenStats;
		TArray<TSharedPtr<SWidget>> OpenStatSlate;
		TArray<TWeakObjectPtr<UAsyncAction_ListenForGameplayMessage>> OpenListeners;
		bool bResumeHost = false;
		double ResumeAt = 0.0;

		~FArenaRoundTravelFixture()
		{
			GEngine->NetDriverDefinitions = SavedDrivers;
			GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = SavedGameInstance;
			SeamlessPIE->SetWithCurrentPriority(SavedSeamlessPIE);
		}
	};

	void OpenStatAcrossTravel(FAutomationTestBase* Test, UWorld* World, FArenaRoundTravelFixture& Fixture)
	{
		UClass* StatClass = LoadClass<UArenaScoreboardStatWidget>(nullptr,
			TEXT("/Game/UI/Scoreboard/PlayerRow/W_SB_PlayerStat.W_SB_PlayerStat_C"));
		UArenaScoreboardStatWidget* Stat = CreateWidget<UArenaScoreboardStatWidget>(World->GetFirstPlayerController(), StatClass);
		if (!Test->TestNotNull(TEXT("Create a production stat held across map cleanup"), Stat))
		{
			return;
		}
		Fixture.OpenStats.Emplace(Stat);
		Fixture.OpenStatSlate.Add(Stat->TakeWidget());
		Stat->EnsureScoreListener();
		const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(
			UArenaScoreboardStatWidget::StaticClass(), TEXT("ScoreListener"));
		UAsyncAction_ListenForGameplayMessage* Listener = Cast<UAsyncAction_ListenForGameplayMessage>(
			Property->GetObjectPropertyValue_InContainer(Stat));
		Test->TestTrue(TEXT("Held stat is actively subscribed before travel"), Listener && Listener->IsActive());
		Fixture.OpenListeners.Add(Listener);
	}

	void CheckTravelStatCleanup(FAutomationTestBase* Test, FArenaRoundTravelFixture& Fixture)
	{
		// Keep both the UObject and its Slate alive until after this assertion: cleanup must come from travel.
		for (const TWeakObjectPtr<UAsyncAction_ListenForGameplayMessage>& Listener : Fixture.OpenListeners)
		{
			Test->TestTrue(TEXT("World cleanup cancels listeners even when widgets remain retained"),
				!Listener.IsValid() || !Listener->IsActive());
		}
		Fixture.OpenStatSlate.Reset();
		Fixture.OpenStats.Reset();
		Fixture.OpenListeners.Reset();
	}

	void CheckPlayerAbilityLifecycle(FAutomationTestBase* Test, UWorld* World)
	{
		AArenaPlayerState* Player = World->SpawnActor<AArenaPlayerState>();
		UArenaAbilitySystemComponent* ASC = CastChecked<UArenaAbilitySystemComponent>(Player->GetAbilitySystemComponent());
		ASC->InitAbilityActorInfo(Player, Player);
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(UArenaGameplayAbility::StaticClass(), 1));
		ASC->UnregisterComponent();
		Test->TestNull(TEXT("Ordinary unregistration still clears abilities"), ASC->FindAbilitySpecFromHandle(Handle));
		ASC->RegisterComponent();
		Handle = ASC->GiveAbility(FGameplayAbilitySpec(UArenaGameplayAbility::StaticClass(), 1));
		UGameplayAbility* OriginalAbility = ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance();
		Test->TestNotNull(TEXT("Instanced player ability exists"), OriginalAbility);

		UArenaSystemStatics::PlayNextGame(World);
		Test->TestTrue(TEXT("Real seamless handler is active"), World->IsInSeamlessTravel());
		ASC->UnregisterComponent();
		ASC->RegisterComponent();
		FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
		Test->TestTrue(TEXT("Player ability identity survives temporary unregister/register"),
			Spec && Spec->GetPrimaryInstance() == OriginalAbility && IsValid(OriginalAbility));

		ASC->DestroyActiveState();
		Test->TestNull(TEXT("Explicit ability destruction is honored during travel"), ASC->FindAbilitySpecFromHandle(Handle));
		ASC->UnregisterComponent();
		ASC->RegisterComponent();
		Handle = ASC->GiveAbility(FGameplayAbilitySpec(UArenaGameplayAbility::StaticClass(), 1));
		Player->Destroy();
		Test->TestNull(TEXT("Player destruction during travel still clears abilities"), ASC->FindAbilitySpecFromHandle(Handle));

		AActor* NonPlayer = World->SpawnActor<AActor>();
		UArenaAbilitySystemComponent* NonPlayerASC = NewObject<UArenaAbilitySystemComponent>(NonPlayer);
		NonPlayer->AddInstanceComponent(NonPlayerASC);
		NonPlayerASC->RegisterComponent();
		NonPlayerASC->InitAbilityActorInfo(NonPlayer, NonPlayer);
		Handle = NonPlayerASC->GiveAbility(FGameplayAbilitySpec(UArenaGameplayAbility::StaticClass(), 1));
		NonPlayerASC->UnregisterComponent();
		Test->TestNull(TEXT("Non-player cleanup is unaffected during travel"), NonPlayerASC->FindAbilitySpecFromHandle(Handle));
		NonPlayer->Destroy();
	}

	class FArenaCheckRoundTravel : public IAutomationLatentCommand
	{
	public:
		FArenaCheckRoundTravel(FAutomationTestBase* InTest, TSharedRef<FArenaRoundTravelFixture> InFixture, bool bInScoreboardSoak = false)
			: Test(InTest)
			, Fixture(InFixture)
			, bScoreboardSoak(bInScoreboardSoak)
		{
		}

		virtual bool Update() override
		{
			if (GetCurrentRunTime() > (bScoreboardSoak ? 300.0 : 90.0))
			{
				Test->AddError(FString::Printf(TEXT("Round travel timed out after %d restarts"), CompletedRestarts));
				return true;
			}

			UWorld* Server = nullptr;
			TArray<UWorld*> Clients;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType != EWorldType::PIE || !World)
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
			if (Server && Fixture->bResumeHost && FPlatformTime::Seconds() >= Fixture->ResumeAt)
			{
				Server->SetSeamlessTravelMidpointPause(false);
				Fixture->bResumeHost = false;
			}
			if (Server && bScoreboardSoak && !Server->IsInSeamlessTravel())
			{
				for (FConstPlayerControllerIterator It = Server->GetPlayerControllerIterator(); It; ++It)
				{
					if (AArenaPlayerState* State = It->Get()->GetPlayerState<AArenaPlayerState>())
					{
						if (State->GetGenericTeamId().GetId() != 1)
						{
							State->SetGenericTeamId(FGenericTeamId(1));
							State->ForceNetUpdate();
						}
					}
				}
			}
			if (!Server || Clients.Num() != 2 || !Server->HasBegunPlay() || Server->IsInSeamlessTravel())
			{
				return false;
			}
			for (UWorld* Client : Clients)
			{
				if (!Client->HasBegunPlay() || Client->IsInSeamlessTravel() ||
					!Client->GetFirstPlayerController() || !Client->GetNetDriver()->ServerConnection ||
					(bScoreboardSoak && !Client->GetFirstPlayerController()->PlayerState) ||
					Client->GetNetDriver()->ServerConnection->GetConnectionState() != USOCK_Open)
				{
					return false;
				}
				if (bScoreboardSoak)
				{
					const AArenaPlayerState* State = Client->GetFirstPlayerController()->GetPlayerState<AArenaPlayerState>();
					if (!State || State->GetGenericTeamId().GetId() != 1)
					{
						return false;
					}
				}
			}
			UNetDriver* Driver = Server->GetNetDriver();
			if (Driver->ClientConnections.Num() != 2)
			{
				return false;
			}
			for (UNetConnection* Connection : Driver->ClientConnections)
			{
				if (!Connection->PlayerController || !Connection->PlayerController->HasClientLoadedCurrentWorld())
				{
					return false;
				}
			}

			if (!bStarted)
			{
				GEngine->GetWorldContextFromWorldChecked(Server).LastURL.AddOption(TEXT("game=/Script/ArenaEditor.ArenaRoundTravelTestGameMode"));
				APlayerController* Host = Server->GetFirstPlayerController();
				UArenaAbilitySystemComponent* ASC = NewObject<UArenaAbilitySystemComponent>(Host);
				Host->AddInstanceComponent(ASC);
				ASC->RegisterComponent();
				ASC->InitAbilityActorInfo(Host, Host);
				Fixture->RetainedASC = ASC;
				Fixture->RetainedAbility = ASC->GiveAbility(FGameplayAbilitySpec(UArenaGameplayAbility::StaticClass(), 1));
				Test->TestNotNull(TEXT("Retained actor has an ability before travel"), ASC->FindAbilitySpecFromHandle(Fixture->RetainedAbility));
				Fixture->Drivers.Add(Driver);
				for (UNetConnection* Connection : Driver->ClientConnections)
				{
					Fixture->Connections.Add(Connection);
				}
				for (UWorld* Client : Clients)
				{
					Fixture->Drivers.Add(Client->GetNetDriver());
					Fixture->Connections.Add(Client->GetNetDriver()->ServerConnection);
				}
				bStarted = true;
			}
			else
			{
				if (Server == Fixture->PreviousServer.Get())
				{
					return false;
				}
				if (bScoreboardSoak)
				{
					CheckTravelStatCleanup(Test, Fixture.Get());
				}
				for (const TWeakObjectPtr<UNetConnection>& Connection : Fixture->Connections)
				{
					if (!Test->TestTrue(TEXT("Original connection survives same-map restart"),
						Connection.IsValid() && Connection->GetConnectionState() == USOCK_Open))
					{
						return true;
					}
				}
				Test->TestTrue(TEXT("Listen driver is retained"), Fixture->Drivers[0].Get() == Driver);
				if (!Test->TestTrue(TEXT("Temporary component unregistration preserves ability subobjects"),
					Fixture->RetainedASC.IsValid() && Fixture->RetainedASC->FindAbilitySpecFromHandle(Fixture->RetainedAbility) != nullptr))
				{
					return true;
				}
				++CompletedRestarts;
				Test->AddInfo(FString::Printf(TEXT("Completed restart %d with both remote players connected"), CompletedRestarts));
				if (CompletedRestarts == (bScoreboardSoak ? 12 : 3))
				{
					if (bScoreboardSoak)
					{
						for (UWorld* Client : Clients)
						{
							CheckScoreboardLifecycle(Test, Client, CompletedRestarts);
						}
					}
					return true;
				}
			}

			if (bScoreboardSoak)
			{
				for (UWorld* Client : Clients)
				{
					if (!CheckScoreboardLifecycle(Test, Client, CompletedRestarts))
					{
						return true;
					}
				}
				for (UWorld* Client : Clients)
				{
					OpenStatAcrossTravel(Test, Client, Fixture.Get());
				}
			}
			Fixture->PreviousServer = Server;
			if (CompletedRestarts == 0)
			{
				CheckPlayerAbilityLifecycle(Test, Server);
			}
			else
			{
				UArenaSystemStatics::PlayNextGame(Server);
			}
			Test->TestTrue(TEXT("Restart begins seamless travel"), Server->IsInSeamlessTravel());
			// Delay the host on the first restart; clients can report loading first.
			if (CompletedRestarts == 0)
			{
				Server->SetSeamlessTravelMidpointPause(true);
				Fixture->bResumeHost = true;
				Fixture->ResumeAt = FPlatformTime::Seconds() + 3.0;
			}
			return false;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FArenaRoundTravelFixture> Fixture;
		bool bStarted = false;
		int32 CompletedRestarts = 0;
		bool bScoreboardSoak = false;
	};

	class FArenaRestoreRoundTravel : public IAutomationLatentCommand
	{
	public:
		explicit FArenaRestoreRoundTravel(TSharedRef<FArenaRoundTravelFixture> InFixture)
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
		TSharedPtr<FArenaRoundTravelFixture> Fixture;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaRoundTravelTest, "Arena.Network.RoundRestart.PreservesConnections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static bool ScheduleRoundTravel(FAutomationTestBase* Test, bool bScoreboardSoak)
{
	if (GEditor->PlayWorld)
	{
		Test->AddError(TEXT("Run in an isolated editor without PIE."));
		return false;
	}
	TSharedRef<FArenaRoundTravelFixture> Fixture = MakeShared<FArenaRoundTravelFixture>();
	Fixture->SeamlessPIE->SetWithCurrentPriority(1);
	GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = FSoftClassPath(UGameInstance::StaticClass());
	FNetDriverDefinition Driver;
	Driver.DefName = NAME_GameNetDriver;
	Driver.DriverClassName = TEXT("/Script/OnlineSubsystemUtils.IpNetDriver");
	Driver.DriverClassNameFallback = Driver.DriverClassName;
	GEngine->NetDriverDefinitions.RemoveAll([](const FNetDriverDefinition& Item) { return Item.DefName == NAME_GameNetDriver; });
	GEngine->NetDriverDefinitions.Add(Driver);
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Engine/Maps/Entry"));
	ULevelEditorPlaySettings* PlaySettings = NewObject<ULevelEditorPlaySettings>();
	PlaySettings->SetPlayNetMode(PIE_ListenServer);
	PlaySettings->SetPlayNumberOfClients(3);
	PlaySettings->SetRunUnderOneProcess(true);
	PlaySettings->SetServerPort(17949);
	PlaySettings->SetClientWindowSize(FIntPoint(320, 240));
	PlaySettings->AddToRoot();
	FRequestPlaySessionParams Params;
	Params.EditorPlaySettings = PlaySettings;
	Params.GameModeOverride = AArenaRoundTravelTestGameMode::StaticClass();
	Params.bAllowOnlineSubsystem = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
	ADD_LATENT_AUTOMATION_COMMAND(FArenaCheckRoundTravel(Test, Fixture, bScoreboardSoak));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FArenaRestoreRoundTravel(Fixture));
	return true;
}

bool FArenaRoundTravelTest::RunTest(const FString&)
{
	return ScheduleRoundTravel(this, false);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaScoreboardRoundSoakTest, "Arena.UI.Scoreboard.TwelveRoundSoak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaScoreboardRoundSoakTest::RunTest(const FString&)
{
	return ScheduleRoundTravel(this, true);
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaReloadedClassNetGUIDTest, "Arena.Network.RoundRestart.ReloadedClassNetGUID",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaReloadedClassNetGUIDTest::RunTest(const FString&)
{
	IConsoleVariable* Remap = IConsoleManager::Get().FindConsoleVariable(TEXT("net.AllowClientRemapCacheObject"));
	if (!TestNotNull(TEXT("Engine supports client cache remapping"), Remap))
	{
		return false;
	}
	const int32 ConfiguredRemap = Remap->GetInt();
	TestEqual(TEXT("Project enables client cache remapping"), ConfiguredRemap, 1);
	ON_SCOPE_EXIT
	{
		Remap->SetWithCurrentPriority(ConfiguredRemap);
	};

	UClass* DashClass = LoadObject<UClass>(nullptr,
		TEXT("/Game/Blueprints/AbilitySystem/Abilities/Action/GA_Hero_Dash.GA_Hero_Dash_C"));
	if (!TestNotNull(TEXT("Load the ability class named in the disconnect log"), DashClass))
	{
		return false;
	}
	TStrongObjectPtr<UIpNetDriver> Driver(NewObject<UIpNetDriver>());

	for (const bool bResolveReloadedPath : {true, false})
	{
		for (const bool bEnableRemap : {false, true})
		{
			// Exercise the failure control first, then the configured production behavior.
			Remap->SetWithCurrentPriority(bEnableRemap ? ConfiguredRemap : 0);
			FArenaClientGuidCache Cache(Driver.Get());
			Cache.SetAsyncLoadMode(FNetGUIDCache::EAsyncLoadMode::ForceDisable);
			const FNetworkGUID PackageGUID = FNetworkGUID::CreateFromIndex(1, true);
			const FNetworkGUID ClassGUID = FNetworkGUID::CreateFromIndex(2, true);
			Cache.RegisterNetGUIDFromPath_Client(PackageGUID, DashClass->GetOutermost()->GetName(),
				FNetworkGUID(), 0, false, false);
			Cache.RegisterNetGUIDFromPath_Client(ClassGUID, DashClass->GetName(), PackageGUID, 0, false, false);
			TestTrue(TEXT("Real client cache resolves the ability class before travel"),
				Cache.GetObjectFromNetGUID(ClassGUID, false) == DashClass);

			// Inject only the unload/age boundary; resolution and expiry use real engine code.
			// The log shows a read-only class remapped after one travel, then missing next round.
			const double ExpiredTimestamp = FPlatformTime::Seconds() - 91.0;
			Cache.ObjectLookup.FindChecked(ClassGUID).ReadOnlyTimestamp = ExpiredTimestamp;
			if (bResolveReloadedPath)
			{
				Cache.ObjectLookup.FindChecked(PackageGUID).ReadOnlyTimestamp = ExpiredTimestamp;
				Cache.ObjectLookup.FindChecked(PackageGUID).Object.Reset();
				Cache.ObjectLookup.FindChecked(ClassGUID).Object.Reset();
				Cache.NetGUIDLookup.Empty();
				TestTrue(TEXT("Reloaded class resolves through the original exported path"),
					Cache.GetObjectFromNetGUID(ClassGUID, false) == DashClass);
			}
			else
			{
				const FNetworkGUID ReusedGUID = Cache.GetOrAssignNetGUID(DashClass);
				TestEqual(TEXT("Existing class retains its GUID only when remapping is enabled"),
					ReusedGUID == ClassGUID, bEnableRemap);
			}

			Cache.CleanReferences();
			TestEqual(TEXT("Reused ability class survives expired-cache cleanup only with remapping"),
				Cache.IsGUIDRegistered(ClassGUID), bEnableRemap);
			TestEqual(TEXT("Subobject class is resolvable after cleanup only with remapping"),
				Cache.GetObjectFromNetGUID(ClassGUID, false) == DashClass, bEnableRemap);
		}
	}
	return true;
}
#endif
