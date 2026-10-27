// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaRoundTravelTestGameMode.h"

AArenaRoundTravelTestGameMode::AArenaRoundTravelTestGameMode()
{
	bStartPlayersAsSpectators = true;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaGameplayAbility.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameMapsSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Player/ArenaPlayerState.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "System/ArenaSystemStatics.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
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
		bool bResumeHost = false;
		double ResumeAt = 0.0;

		~FArenaRoundTravelFixture()
		{
			GEngine->NetDriverDefinitions = SavedDrivers;
			GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = SavedGameInstance;
			SeamlessPIE->SetWithCurrentPriority(SavedSeamlessPIE);
		}
	};

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
		FArenaCheckRoundTravel(FAutomationTestBase* InTest, TSharedRef<FArenaRoundTravelFixture> InFixture)
			: Test(InTest)
			, Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			if (GetCurrentRunTime() > 90.0)
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
			if (!Server || Clients.Num() != 2 || !Server->HasBegunPlay() || Server->IsInSeamlessTravel())
			{
				return false;
			}
			for (UWorld* Client : Clients)
			{
				if (!Client->HasBegunPlay() || Client->IsInSeamlessTravel() ||
					!Client->GetFirstPlayerController() || !Client->GetNetDriver()->ServerConnection ||
					Client->GetNetDriver()->ServerConnection->GetConnectionState() != USOCK_Open)
				{
					return false;
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
				if (CompletedRestarts == 3)
				{
					return true;
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

bool FArenaRoundTravelTest::RunTest(const FString&)
{
	if (GEditor->PlayWorld)
	{
		AddError(TEXT("Run in an isolated editor without PIE."));
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
	ADD_LATENT_AUTOMATION_COMMAND(FArenaCheckRoundTravel(this, Fixture));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FArenaRestoreRoundTravel(Fixture));
	return true;
}
#endif
