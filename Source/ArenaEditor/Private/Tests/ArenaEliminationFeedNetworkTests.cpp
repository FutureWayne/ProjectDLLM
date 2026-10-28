// Copyright Ludens Studio. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ArenaEliminationFeedTestTypes.h"

#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "GameState/ArenaGameState.h"
#include "Messages/ArenaEliminationFeedRelay.h"
#include "Messages/ArenaVerbMessage.h"
#include "Misc/AutomationTest.h"
#include "PlayInEditorDataTypes.h"
#include "Player/ArenaPlayerState.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/ArenaEliminationFeedEntry.h"
#include "UObject/StrongObjectPtr.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

namespace ArenaEliminationFeedNetworkTests
{
	const TCHAR* GrenadePath = TEXT("/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade/DA_NormalGrenadeDef.DA_NormalGrenadeDef");

	struct FFixture
	{
		TArray<FNetDriverDefinition> SavedDrivers = GEngine->NetDriverDefinitions;
		FSoftClassPath SavedGameInstance = GetDefault<UGameMapsSettings>()->GameInstanceClass;
		TArray<TStrongObjectPtr<UArenaEliminationFeedWidget>> Feeds;
		TStrongObjectPtr<UArenaEliminationFeedRelay> Relay;
		TStrongObjectPtr<UArenaGrenadeDefinitionData> Grenade;

		~FFixture()
		{
			GEngine->NetDriverDefinitions = SavedDrivers;
			GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = SavedGameInstance;
		}
	};

	class FCheckDelivery : public IAutomationLatentCommand
	{
	public:
		FCheckDelivery(FAutomationTestBase* InTest, TSharedRef<FFixture> InFixture)
			: Test(InTest), Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			if (GetCurrentRunTime() > 40.0)
			{
				Test->AddError(FString::Printf(TEXT("Elimination feed network test timed out at stage %d"), Stage));
				return true;
			}
			UWorld* Server = nullptr;
			TArray<UWorld*> Worlds;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType == EWorldType::PIE && World && World->HasBegunPlay())
				{
					Worlds.Add(World);
					if (World->GetNetMode() == NM_ListenServer)
					{
						Server = World;
					}
				}
			}
			if (!Server || Worlds.Num() != 3)
			{
				return false;
			}
			AArenaGameState* GameState = Server->GetGameState<AArenaGameState>();
			if (!GameState || GameState->PlayerArray.Num() != 3)
			{
				return false;
			}
			if (Stage == 0)
			{
				for (UWorld* World : Worlds)
				{
					APlayerController* Controller = World->GetFirstPlayerController();
					AArenaGameState* Copy = World->GetGameState<AArenaGameState>();
					if (!Controller || !Controller->IsLocalController() || !Copy || Copy->PlayerArray.Num() != 3)
					{
						return false;
					}
				}
				UClass* FeedClass = LoadClass<UArenaEliminationFeedWidget>(nullptr, TEXT("/Game/UI/Notifications/EliminationFeed/W_EliminationFeed.W_EliminationFeed_C"));
				UClass* RelayClass = LoadClass<UArenaEliminationFeedRelay>(nullptr, TEXT("/Game/Blueprints/Accolades/B_EliminationFeedRelay.B_EliminationFeedRelay_C"));
				if (!Test->TestNotNull(TEXT("Load production feed"), FeedClass) || !Test->TestNotNull(TEXT("Load production relay"), RelayClass))
				{
					return true;
				}
				for (UWorld* World : Worlds)
				{
					UArenaEliminationFeedWidget* Feed = CreateWidget<UArenaEliminationFeedWidget>(World->GetFirstPlayerController(), FeedClass);
					Feed->AddToViewport();
					Fixture->Feeds.Emplace(Feed);
				}
				UArenaEliminationFeedRelay* Relay = NewObject<UArenaEliminationFeedRelay>(GameState, RelayClass);
				GameState->AddInstanceComponent(Relay);
				Relay->RegisterComponent();
				Relay->StartListening();
				Relay->StartListening();
				Fixture->Relay.Reset(Relay);
				UClass* StreakClass = LoadClass<UActorComponent>(nullptr, TEXT("/Game/Blueprints/Accolades/B_ElimStreakProcessor.B_ElimStreakProcessor_C"));
				if (!Test->TestNotNull(TEXT("Load authoritative streak processor"), StreakClass))
				{
					return true;
				}
				UActorComponent* Streak = NewObject<UActorComponent>(GameState, StreakClass);
				GameState->AddInstanceComponent(Streak);
				Streak->RegisterComponent();
				CastChecked<AArenaPlayerState>(GameState->PlayerArray[0])->SetGenericTeamId(FGenericTeamId(1));
				CastChecked<AArenaPlayerState>(GameState->PlayerArray[1])->SetGenericTeamId(FGenericTeamId(2));
				Stage = 1;
				return false;
			}
			if (Stage == 1)
			{
				FArenaVerbMessage Message;
				Message.Verb = FGameplayTag::RequestGameplayTag(TEXT("Arena.Elimination.Message"));
				Message.Instigator = GameState->PlayerArray[0];
				Message.Target = GameState->PlayerArray[1];
				Message.ContextTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Gameplay.Damage.FromEnemy")));
				Fixture->Grenade.Reset(LoadObject<UArenaGrenadeDefinitionData>(nullptr, GrenadePath));
				if (!Test->TestNotNull(TEXT("Load production grenade definition"), Fixture->Grenade.Get()))
				{
					return true;
				}
				Message.ContextObject = Fixture->Grenade.Get();
				for (int32 Index = 0; Index < 3; ++Index)
				{
					UGameplayMessageSubsystem::Get(Server).BroadcastMessage(Message.Verb, Message);
				}
				Stage = 2;
				return false;
			}
			if (Stage == 2)
			{
				for (const TStrongObjectPtr<UArenaEliminationFeedWidget>& Feed : Fixture->Feeds)
				{
					UListView* List = CastChecked<UListView>(Feed->GetWidgetFromName(TEXT("ToastListWidget")));
					if (List->GetNumItems() < 3)
					{
						return false;
					}
					// NullRHI does not paint viewports; tick the real list layout to instantiate rows.
					TSharedRef<SWidget> SlateList = List->TakeWidget();
					SlateList->SlatePrepass();
					SlateList->Tick(FGeometry::MakeRoot(FVector2D(640, 480), FSlateLayoutTransform()), FPlatformTime::Seconds(), 0.016f);
					UArenaEliminationFeedEntry* Row = List->GetEntryWidgetFromItem<UArenaEliminationFeedEntry>(List->GetItemAt(2));
					if (!Row)
					{
						return false;
					}
				}
				for (const TStrongObjectPtr<UArenaEliminationFeedWidget>& Feed : Fixture->Feeds)
				{
					UListView* List = CastChecked<UListView>(Feed->GetWidgetFromName(TEXT("ToastListWidget")));
					Test->TestEqual(TEXT("Host and each remote client receive each entry exactly once"), List->GetNumItems(), 3);
					UArenaEliminationFeedItem* Item = CastChecked<UArenaEliminationFeedItem>(List->GetItemAt(0));
					Test->TestTrue(TEXT("Context survives reliable multicast"), Item->EliminationType == EArenaEliminationType::Enemy);
					Test->TestTrue(TEXT("Grenade definition survives reliable multicast"), Item->GrenadeDefinition == Fixture->Grenade.Get());
					UArenaEliminationFeedEntry* Row = List->GetEntryWidgetFromItem<UArenaEliminationFeedEntry>(Item);
					UTextBlock* Name = Cast<UTextBlock>(Row->GetWidgetFromName(TEXT("AttackerName")));
					UWidgetSwitcher* Icons = Cast<UWidgetSwitcher>(Row->GetWidgetFromName(TEXT("WeaponIconSwitcher")));
					const FText ExpectedName = UArenaEliminationFeedEntry::ClampDisplayName(Item->AttackerName, Row->GetMaxNameLength());
					Test->TestTrue(TEXT("Production entry receives native snapshot text within the name cap"), Name && Name->GetText().EqualTo(ExpectedName));
					Test->TestTrue(TEXT("Production WBP selects enemy icon"), Icons && Icons->GetActiveWidget() == Row->GetWidgetFromName(TEXT("EnemyKill")));
					UArenaEliminationFeedItem* Third = CastChecked<UArenaEliminationFeedItem>(List->GetItemAt(2));
					Test->TestEqual(TEXT("Authoritative triple streak survives multicast"), Third->EliminationStreak, 3);
					UArenaEliminationFeedEntry* StreakRow = List->GetEntryWidgetFromItem<UArenaEliminationFeedEntry>(Third);
					UTextBlock* StreakText = Cast<UTextBlock>(StreakRow->GetWidgetFromName(TEXT("StreakText")));
					Test->TestTrue(TEXT("WBP displays triple-streak count"), StreakText && StreakText->GetText().ToString() == TEXT("3"));
					Test->TestTrue(TEXT("WBP enables streak badge"), StreakRow->GetWidgetFromName(TEXT("StreakBadge"))->GetVisibility() == ESlateVisibility::HitTestInvisible);
				}
				Stage = 3;
				return false;
			}
			for (const TStrongObjectPtr<UArenaEliminationFeedWidget>& Feed : Fixture->Feeds)
			{
				UListView* List = CastChecked<UListView>(Feed->GetWidgetFromName(TEXT("ToastListWidget")));
				if (List->GetNumItems() != 0)
				{
					return false;
				}
			}
			Test->TestTrue(TEXT("All network feed entries expire independently"), true);
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FFixture> Fixture;
		int32 Stage = 0;
	};

	class FRestoreFixture : public IAutomationLatentCommand
	{
	public:
		explicit FRestoreFixture(TSharedRef<FFixture> InFixture) : Fixture(InFixture) {}

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

	class FReleaseWorldReferences : public IAutomationLatentCommand
	{
	public:
		explicit FReleaseWorldReferences(TSharedRef<FFixture> InFixture)
			: Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			for (const TStrongObjectPtr<UArenaEliminationFeedWidget>& Feed : Fixture->Feeds)
			{
				Feed->RemoveFromParent();
			}
			Fixture->Feeds.Reset();
			Fixture->Relay.Reset();
			return true;
		}

	private:
		TSharedRef<FFixture> Fixture;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaEliminationFeedNetworkTest, "Arena.UI.EliminationFeed.Network.ReliableDelivery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaEliminationFeedNetworkTest::RunTest(const FString& Parameters)
{
	using namespace ArenaEliminationFeedNetworkTests;
	if (GEditor->PlayWorld)
	{
		AddError(TEXT("Run in an isolated editor without an active PIE session."));
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
	Settings->SetServerPort(17939);
	Settings->SetClientWindowSize(FIntPoint(640, 480));
	Settings->AddToRoot(); // Balanced by FStartPIEForAutomationCommand.
	FRequestPlaySessionParams Request;
	Request.EditorPlaySettings = Settings;
	Request.GameModeOverride = AArenaEliminationFeedTestGameMode::StaticClass();
	Request.bAllowOnlineSubsystem = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Request));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckDelivery(this, Fixture));
	ADD_LATENT_AUTOMATION_COMMAND(FReleaseWorldReferences(Fixture));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FRestoreFixture(Fixture));
	return true;
}
#endif
