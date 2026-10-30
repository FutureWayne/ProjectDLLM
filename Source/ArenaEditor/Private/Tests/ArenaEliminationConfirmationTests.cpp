// Copyright Ludens Studio. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Messages/ArenaVerbMessage.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Player/ArenaPlayerState.h"
#include "UI/ArenaEliminationFeedEntry.h"
#include "UI/Widget/ArenaEliminationFeedWidget.h"

namespace ArenaEliminationConfirmationTests
{
	const TCHAR* ConfirmationPath = TEXT("/Game/UI/Notifications/EliminationConfirmation/W_EliminationConfirmation.W_EliminationConfirmation_C");
	const TCHAR* OverlayPath = TEXT("/Game/UI/Hud/WBP_CharacterOverlay.WBP_CharacterOverlay_C");
	const TCHAR* StreakPath = TEXT("/Game/Blueprints/Accolades/B_ElimStreakProcessor.B_ElimStreakProcessor_C");

	/** Sends the raw server elimination message; the production streak processor turns it into the feed message. */
	void SendElimination(UWorld* World, UObject* Attacker, UObject* Victim, const TCHAR* Context)
	{
		FArenaVerbMessage Message;
		Message.Verb = FGameplayTag::RequestGameplayTag(TEXT("Arena.Elimination.Message"));
		Message.Instigator = Attacker;
		Message.Target = Victim;
		Message.ContextTags.AddTag(FGameplayTag::RequestGameplayTag(Context));
		UGameplayMessageSubsystem::Get(World).BroadcastMessage(Message.Verb, Message);
	}

	void TickWorld(UWorld* World, float Seconds)
	{
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(Seconds / 0.02f));
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			++GFrameCounter;
			World->Tick(LEVELTICK_All, Seconds / Steps);
		}
	}

	AArenaPlayerState* SpawnPlayer(UWorld* World, const TCHAR* Name, uint8 Team)
	{
		AArenaPlayerState* Player = World->SpawnActor<AArenaPlayerState>();
		Player->SetPlayerName(Name);
		Player->SetGenericTeamId(FGenericTeamId(Team));
		return Player;
	}

	const UArenaEliminationFeedItem* GetItem(const UListView* List, int32 Index)
	{
		return Index < List->GetNumItems() ? Cast<UArenaEliminationFeedItem>(List->GetItemAt(Index)) : nullptr;
	}

	UArenaEliminationFeedEntry* RealizeRow(UListView* List, int32 Index)
	{
		// Tests do not paint viewports; tick the real list layout to instantiate the production rows.
		TSharedRef<SWidget> SlateList = List->TakeWidget();
		SlateList->SlatePrepass();
		SlateList->Tick(FGeometry::MakeRoot(FVector2D(900.0, 170.0), FSlateLayoutTransform()), FPlatformTime::Seconds(), 0.016f);
		return List->GetEntryWidgetFromItem<UArenaEliminationFeedEntry>(List->GetItemAt(Index));
	}

	FString GetRowText(const UArenaEliminationFeedEntry* Row, const TCHAR* WidgetName)
	{
		const UTextBlock* Text = Row ? Cast<UTextBlock>(Row->GetWidgetFromName(WidgetName)) : nullptr;
		return Text ? Text->GetText().ToString() : FString();
	}

	bool IsRowWidgetShown(const UArenaEliminationFeedEntry* Row, const TCHAR* WidgetName)
	{
		const UWidget* Widget = Row ? Row->GetWidgetFromName(WidgetName) : nullptr;
		return Widget && Widget->GetVisibility() != ESlateVisibility::Collapsed && Widget->GetVisibility() != ESlateVisibility::Hidden;
	}

	/** The confirmation appends at the bottom, so the newest entry is the last list item. */
	const UArenaEliminationFeedItem* GetNewest(const UListView* List)
	{
		return List->GetNumItems() > 0 ? GetItem(List, List->GetNumItems() - 1) : nullptr;
	}

	float GetStackOffset(const UListView* List)
	{
		return static_cast<float>(List->GetRenderTransform().Translation.Y);
	}

	/** Drives the production WBP tick and its stack animation without painting a viewport. */
	void TickSlate(const TSharedRef<SWidget>& Widget, float Seconds)
	{
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(Seconds / 0.016f));
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Widget->Tick(FGeometry::MakeRoot(FVector2D(560.0, 116.0), FSlateLayoutTransform()), FPlatformTime::Seconds(), Seconds / Steps);
		}
	}

	template <typename TProperty, typename TValue>
	bool ReadDefault(const UClass* Class, const TCHAR* Name, TValue& OutValue)
	{
		const TProperty* Property = CastField<TProperty>(Class->FindPropertyByName(Name));
		if (!Property)
		{
			return false;
		}
		OutValue = Property->GetPropertyValue_InContainer(Class->GetDefaultObject());
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaEliminationConfirmationTest, "Arena.UI.EliminationFeed.PersonalConfirmation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaEliminationConfirmationTest::RunTest(const FString& Parameters)
{
	using namespace ArenaEliminationConfirmationTests;
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	ON_SCOPE_EXIT
	{
		World->BeginTearingDown();
		World->EndPlay(EEndPlayReason::Quit);
		Instance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};

	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	Instance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	ON_SCOPE_EXIT { Instance->RemoveLocalPlayer(LocalPlayer); };

	AArenaPlayerState* Local = SpawnPlayer(World, TEXT("Local"), 1);
	AArenaPlayerState* Enemy = SpawnPlayer(World, TEXT("Enemy"), 2);
	AArenaPlayerState* OtherEnemy = SpawnPlayer(World, TEXT("OtherEnemy"), 2);
	AArenaPlayerState* Teammate = SpawnPlayer(World, TEXT("Teammate"), 1);
	AArenaPlayerState* Bystander = SpawnPlayer(World, TEXT("Bystander"), 1);
	Controller->SetPlayerState(Local);

	UClass* StreakClass = LoadClass<UActorComponent>(nullptr, StreakPath);
	UClass* ConfirmationClass = LoadClass<UArenaEliminationFeedWidget>(nullptr, ConfirmationPath);
	if (!TestNotNull(TEXT("Load production streak processor"), StreakClass) || !TestNotNull(TEXT("Production confirmation derives from the native feed"), ConfirmationClass))
	{
		return false;
	}
	AActor* StreakOwner = World->SpawnActor<AActor>();
	UActorComponent* Streak = NewObject<UActorComponent>(StreakOwner, StreakClass);
	StreakOwner->AddInstanceComponent(Streak);
	Streak->RegisterComponent();

	UArenaEliminationFeedWidget* Confirmation = CreateWidget<UArenaEliminationFeedWidget>(Controller, ConfirmationClass);
	TSharedPtr<SWidget> SlateConfirmation = Confirmation->TakeWidget();
	ON_SCOPE_EXIT { SlateConfirmation.Reset(); };
	UListView* List = Cast<UListView>(Confirmation->GetWidgetFromName(TEXT("ToastListWidget")));
	const UWidget* VisibilityOverlay = Confirmation->GetWidgetFromName(TEXT("FeedVisibilityOvr"));
	if (!TestNotNull(TEXT("Confirmation list binds"), List) || !TestNotNull(TEXT("Confirmation visibility presentation binds"), VisibilityOverlay))
	{
		return false;
	}
	TestTrue(TEXT("An empty confirmation stack is collapsed"), VisibilityOverlay->GetVisibility() == ESlateVisibility::Collapsed);

	int32 MaxEntries = 0;
	double Pitch = 0.0;
	bool bNewestFirst = true;
	if (!TestTrue(TEXT("Read confirmation stack configuration"),
		ReadDefault<FIntProperty>(ConfirmationClass, TEXT("MaxEntries"), MaxEntries)
		&& ReadDefault<FDoubleProperty>(ConfirmationClass, TEXT("StackRiseRowPitch"), Pitch)
		&& ReadDefault<FBoolProperty>(ConfirmationClass, TEXT("bNewestFirst"), bNewestFirst)))
	{
		return false;
	}
	TestTrue(TEXT("Confirmation stacks several entries that rise from the bottom row"), MaxEntries == 3 && Pitch > 0.0f && !bNewestFirst);

	SendElimination(World, Bystander, Enemy, TEXT("Gameplay.Damage.FromEnemy"));
	SendElimination(World, nullptr, Enemy, TEXT("Gameplay.Damage.SelfDestruct"));
	SendElimination(World, nullptr, Local, TEXT("Gameplay.Damage.SelfDestruct"));
	TestEqual(TEXT("Other players' eliminations and killer-less deaths are not shown"), List->GetNumItems(), 0);

	SendElimination(World, Local, Enemy, TEXT("Gameplay.Damage.FromEnemy"));
	const UArenaEliminationFeedItem* FirstKill = GetNewest(List);
	TestTrue(TEXT("Enemy confirmation names the victim with the authoritative count"),
		FirstKill && FirstKill->VictimName.ToString() == TEXT("Enemy") && FirstKill->EliminationType == EArenaEliminationType::Enemy && FirstKill->EliminationStreak == 1);
	TestTrue(TEXT("A nonempty confirmation stack is shown"), VisibilityOverlay->GetVisibility() != ESlateVisibility::Collapsed);
	TestTrue(TEXT("The first confirmation enters from below the bottom row"), FMath::IsNearlyEqual(GetStackOffset(List), MaxEntries * Pitch, 0.5f));
	TickSlate(SlateConfirmation.ToSharedRef(), 1.0f);
	TestTrue(TEXT("The first confirmation settles on the bottom row"), FMath::IsNearlyEqual(GetStackOffset(List), (MaxEntries - 1) * Pitch, 0.5f));

	SendElimination(World, Local, OtherEnemy, TEXT("Gameplay.Damage.FromEnemy"));
	const UArenaEliminationFeedItem* SecondKill = GetNewest(List);
	TestTrue(TEXT("The newest confirmation is appended below the earlier one"),
		SecondKill && SecondKill->VictimName.ToString() == TEXT("OtherEnemy") && List->GetNumItems() == 2 && GetItem(List, 0) == FirstKill);
	TestTrue(TEXT("Enemy streak increments for consecutive kills"), SecondKill && SecondKill->EliminationStreak == 2);
	TestTrue(TEXT("Earlier confirmations do not jump when a new one arrives"), FMath::IsNearlyEqual(GetStackOffset(List), (MaxEntries - 1) * Pitch, 0.5f));
	TickSlate(SlateConfirmation.ToSharedRef(), 0.05f);
	const float MidRise = GetStackOffset(List);
	TestTrue(TEXT("The stack glides up rather than snapping"), MidRise < (MaxEntries - 1) * Pitch - 0.5f && MidRise > (MaxEntries - 2) * Pitch + 0.5f);
	TickSlate(SlateConfirmation.ToSharedRef(), 1.0f);
	TestTrue(TEXT("The stack rises by exactly one row"), FMath::IsNearlyEqual(GetStackOffset(List), (MaxEntries - 2) * Pitch, 0.5f));
	UArenaEliminationFeedEntry* EnemyRow = RealizeRow(List, List->GetNumItems() - 1);
	if (!TestNotNull(TEXT("Production confirmation row is generated"), EnemyRow))
	{
		return false;
	}
	const FText ExpectedName = UArenaEliminationFeedEntry::ClampDisplayName(SecondKill->VictimName, EnemyRow->GetMaxNameLength());
	TestEqual(TEXT("Row shows the victim name within the name cap"), GetRowText(EnemyRow, TEXT("VictimName")), ExpectedName.ToString());
	TestEqual(TEXT("The earlier row keeps its own victim name"), GetRowText(RealizeRow(List, 0), TEXT("VictimName")), FString(TEXT("Enemy")));
	const FString EnemyLabel = GetRowText(EnemyRow, TEXT("TypeLabel"));
	TestFalse(TEXT("Row shows a kill type label"), EnemyLabel.IsEmpty());
	TestEqual(TEXT("Row shows the streak count"), GetRowText(EnemyRow, TEXT("StreakText")), FString(TEXT("2")));
	TestTrue(TEXT("Row shows the streak count from two kills"), IsRowWidgetShown(EnemyRow, TEXT("StreakBadge")));
	TestNull(TEXT("Row carries no type icon"), EnemyRow->GetWidgetFromName(TEXT("TypeIconSwitcher")));
	TestNull(TEXT("Row carries no streak emblem"), EnemyRow->GetWidgetFromName(TEXT("StreakEmblem")));

	SendElimination(World, Local, Teammate, TEXT("Gameplay.Damage.FromAlly"));
	const UArenaEliminationFeedItem* AllyKill = GetNewest(List);
	TestTrue(TEXT("Teammate confirmation uses its own count"), AllyKill && AllyKill->EliminationType == EArenaEliminationType::Ally && AllyKill->EliminationStreak == 1);
	UArenaEliminationFeedEntry* AllyRow = RealizeRow(List, List->GetNumItems() - 1);
	TestEqual(TEXT("Teammate row names the teammate"), GetRowText(AllyRow, TEXT("VictimName")), FString(TEXT("Teammate")));
	TestTrue(TEXT("Teammate row uses its own type label"), !GetRowText(AllyRow, TEXT("TypeLabel")).IsEmpty() && GetRowText(AllyRow, TEXT("TypeLabel")) != EnemyLabel);
	TestFalse(TEXT("A single kill shows no streak count"), IsRowWidgetShown(AllyRow, TEXT("StreakBadge")));
	TickSlate(SlateConfirmation.ToSharedRef(), 1.0f);
	TestTrue(TEXT("A full stack fills every row"), List->GetNumItems() == MaxEntries && FMath::IsNearlyEqual(GetStackOffset(List), 0.0f, 0.5f));

	SendElimination(World, Local, Local, TEXT("Gameplay.Damage.SelfDestruct"));
	const UArenaEliminationFeedItem* SelfKill = GetNewest(List);
	TestTrue(TEXT("Self elimination is confirmed with its own count"), SelfKill && SelfKill->EliminationType == EArenaEliminationType::Self && SelfKill->EliminationStreak == 1);
	TestTrue(TEXT("Capacity evicts the oldest confirmation from the top"),
		List->GetNumItems() == MaxEntries && !List->GetListItems().Contains(FirstKill) && GetItem(List, 0) == SecondKill && GetItem(List, 1) == AllyKill);
	TestTrue(TEXT("Survivors stay in place when the top entry is evicted"), FMath::IsNearlyEqual(GetStackOffset(List), Pitch, 0.5f));
	TickSlate(SlateConfirmation.ToSharedRef(), 1.0f);
	TestTrue(TEXT("A full stack rises one row after eviction"), FMath::IsNearlyEqual(GetStackOffset(List), 0.0f, 0.5f));
	UArenaEliminationFeedEntry* SelfRow = RealizeRow(List, List->GetNumItems() - 1);
	TestTrue(TEXT("Self row uses its own type label"), !GetRowText(SelfRow, TEXT("TypeLabel")).IsEmpty() && GetRowText(SelfRow, TEXT("TypeLabel")) != EnemyLabel);
	TestFalse(TEXT("Self row does not repeat the local player's name"), IsRowWidgetShown(SelfRow, TEXT("VictimName")));
	TestFalse(TEXT("Self elimination is not reported as being eliminated by another player"), SelfKill && SelfKill->bOwningPlayerEliminated);

	SendElimination(World, Enemy, Local, TEXT("Gameplay.Damage.FromEnemy"));
	const UArenaEliminationFeedItem* KilledBy = GetNewest(List);
	TestTrue(TEXT("Being eliminated by another player is shown"), KilledBy && KilledBy->bOwningPlayerEliminated && KilledBy->AttackerName.ToString() == TEXT("Enemy"));
	TestEqual(TEXT("Killed-by confirmation joins the stack"), List->GetNumItems(), MaxEntries);
	UArenaEliminationFeedEntry* KilledByRow = RealizeRow(List, List->GetNumItems() - 1);
	TestEqual(TEXT("Killed-by row uses the Eliminated By label"), GetRowText(KilledByRow, TEXT("TypeLabel")), FString(TEXT("ELIMINATED BY")));
	TestEqual(TEXT("Killed-by row names the killer"), GetRowText(KilledByRow, TEXT("VictimName")), FString(TEXT("Enemy")));
	TestTrue(TEXT("Killed-by row shows the killer name"), IsRowWidgetShown(KilledByRow, TEXT("VictimName")));
	TestFalse(TEXT("Killed-by row shows no streak count"), IsRowWidgetShown(KilledByRow, TEXT("StreakBadge")));
	const UTextBlock* KillerText = KilledByRow ? Cast<UTextBlock>(KilledByRow->GetWidgetFromName(TEXT("VictimName"))) : nullptr;
	const FLinearColor KillerColor = KillerText ? KillerText->GetColorAndOpacity().GetSpecifiedColor() : FLinearColor::Black;
	TestTrue(TEXT("Killer name is grey-white rather than a team colour"),
		KillerColor.R > 0.6f && FMath::Abs(KillerColor.R - KillerColor.G) < 0.05f && FMath::Abs(KillerColor.G - KillerColor.B) < 0.05f);
	TickSlate(SlateConfirmation.ToSharedRef(), 1.0f);

	TickWorld(World, 1.0f);
	TestEqual(TEXT("Confirmations stay readable for their short lifetime"), List->GetNumItems(), MaxEntries);
	SendElimination(World, Teammate, Local, TEXT("Gameplay.Damage.FromAlly"));
	const UArenaEliminationFeedItem* KilledByTeammate = GetNewest(List);
	TestTrue(TEXT("Being eliminated by a teammate also names the killer"), KilledByTeammate && KilledByTeammate->bOwningPlayerEliminated && KilledByTeammate->AttackerName.ToString() == TEXT("Teammate"));
	TickSlate(SlateConfirmation.ToSharedRef(), 1.0f);

	TickWorld(World, 1.6f);
	TestTrue(TEXT("Older confirmations expire first while the newest remains"), List->GetNumItems() == 1 && GetNewest(List) == KilledByTeammate);
	TestTrue(TEXT("The remaining confirmation stays on the bottom row when older ones expire"), FMath::IsNearlyEqual(GetStackOffset(List), (MaxEntries - 1) * Pitch, 0.5f));
	TickWorld(World, 1.0f);
	TestEqual(TEXT("Confirmation expires after its short lifetime"), List->GetNumItems(), 0);
	TestTrue(TEXT("Confirmation collapses after expiry"), VisibilityOverlay->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("An expired stack resets its entry position"), FMath::IsNearlyEqual(GetStackOffset(List), MaxEntries * Pitch, 0.5f));
	SendElimination(World, Local, Enemy, TEXT("Gameplay.Damage.FromEnemy"));
	TestTrue(TEXT("A new confirmation after expiry starts below the bottom row"), List->GetNumItems() == 1 && FMath::IsNearlyEqual(GetStackOffset(List), MaxEntries * Pitch, 0.5f));
	TickSlate(SlateConfirmation.ToSharedRef(), 1.0f);
	TestTrue(TEXT("The restarted stack settles on the bottom row"), FMath::IsNearlyEqual(GetStackOffset(List), (MaxEntries - 1) * Pitch, 0.5f));

	const UWidgetBlueprintGeneratedClass* OverlayClass = LoadObject<UWidgetBlueprintGeneratedClass>(nullptr, OverlayPath);
	const UWidgetTree* OverlayTree = OverlayClass ? OverlayClass->GetWidgetTreeArchetype() : nullptr;
	const UWidget* HudConfirmation = OverlayTree ? OverlayTree->FindWidget(TEXT("W_EliminationConfirmation")) : nullptr;
	const UCanvasPanelSlot* HudSlot = HudConfirmation ? Cast<UCanvasPanelSlot>(HudConfirmation->Slot) : nullptr;
	TestTrue(TEXT("Character overlay hosts the production confirmation"), HudConfirmation && HudConfirmation->IsA(ConfirmationClass));
	TestTrue(TEXT("Confirmation sits horizontally centred below the reticle"),
		HudSlot && HudSlot->GetAnchors().Minimum == FVector2D(0.5, 0.5) && HudSlot->GetAlignment().X == 0.5f && HudSlot->GetPosition().Y > 0.0f);

	// The placed instance must not override the single-entry behaviour; stale overrides queue later kills off-screen.
	if (HudConfirmation)
	{
		const UObject* ClassDefaults = ConfirmationClass->GetDefaultObject();
		for (const TCHAR* PropertyName : { TEXT("MaxEntries"), TEXT("EntryLifetime"), TEXT("bNewestFirst"), TEXT("bOnlyOwningPlayerEliminations"),
			TEXT("bIncludeOwningPlayerDeaths"), TEXT("StackRiseRowPitch"), TEXT("StackRiseSpeed") })
		{
			const FProperty* Property = ConfirmationClass->FindPropertyByName(PropertyName);
			TestTrue(FString::Printf(TEXT("HUD confirmation keeps the class default for %s"), PropertyName),
				Property && Property->Identical_InContainer(HudConfirmation, ClassDefaults));
		}
	}
	return true;
}
#endif
