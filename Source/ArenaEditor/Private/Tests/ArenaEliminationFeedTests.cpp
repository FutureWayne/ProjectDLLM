// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaEliminationFeedTestTypes.h"

#include "Components/ListView.h"
#include "GameState/ArenaGameState.h"
#include "Player/ArenaPlayerState.h"

void UArenaEliminationFeedTestWidget::ConstructForTest()
{
	if (!ToastListWidget)
	{
		ToastListWidget = NewObject<UListView>(this);
	}
	NativeConstruct();
}

void UArenaEliminationFeedTestWidget::ConfigureForTest(float Lifetime, int32 Limit, bool bOnlyOwningPlayer)
{
	EntryLifetime = Lifetime;
	MaxEntries = Limit;
	bOnlyOwningPlayerEliminations = bOnlyOwningPlayer;
}

AArenaEliminationFeedTestGameMode::AArenaEliminationFeedTestGameMode()
{
	GameStateClass = AArenaGameState::StaticClass();
	PlayerStateClass = AArenaPlayerState::StaticClass();
}

#if WITH_DEV_AUTOMATION_TESTS
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Actor/ArenaEffectActor.h"
#include "Character/ArenaHealthComponent.h"
#include "Components/Overlay.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "GameplayEffect.h"
#include "Messages/ArenaEliminationFeedRelay.h"
#include "Messages/ArenaVerbMessage.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/ArenaEliminationFeedEntry.h"
#include "UObject/StrongObjectPtr.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

namespace ArenaEliminationFeedTests
{
	const TCHAR* FeedPath = TEXT("/Game/UI/Notifications/EliminationFeed/W_EliminationFeed.W_EliminationFeed_C");
	const TCHAR* RelayPath = TEXT("/Game/Blueprints/Accolades/B_EliminationFeedRelay.B_EliminationFeedRelay_C");

	FArenaVerbMessage MakeMessage(UObject* Attacker, UObject* Victim, const TCHAR* Context = TEXT("Gameplay.Damage.FromEnemy"))
	{
		FArenaVerbMessage Message;
		Message.Verb = FGameplayTag::RequestGameplayTag(TEXT("Arena.Elimination.Message"));
		Message.Instigator = Attacker;
		Message.Target = Victim;
		Message.ContextTags.AddTag(FGameplayTag::RequestGameplayTag(Context));
		return Message;
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

	UArenaHealthComponent* AddHealth(APawn* Pawn, AArenaPlayerState* Player)
	{
		Pawn->SetPlayerState(Player);
		UArenaAbilitySystemComponent* ASC = Player->GetArenaAbilitySystemComponent();
		ASC->InitAbilityActorInfo(Player, Pawn);
		ASC->AddAttributeSetSubobject(Player->GetArenaHealthSet());
		UArenaHealthComponent* Health = NewObject<UArenaHealthComponent>(Pawn);
		Pawn->AddInstanceComponent(Health);
		Health->RegisterComponent();
		Health->InitializeWithAbilitySystem(ASC);
		return Health;
	}

	void Damage(AArenaPlayerState* Source, AArenaPlayerState* Target, float Amount, const UObject* SourceObject = nullptr)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>();
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UArenaHealthSet::GetDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Amount);
		FGameplayEffectContextHandle Context = Source->GetArenaAbilitySystemComponent()->MakeEffectContext();
		Context.AddSourceObject(SourceObject);
		FGameplayEffectSpec Spec(Effect, Context, 1.0f);
		Source->GetArenaAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(Spec, Target->GetArenaAbilitySystemComponent());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaEliminationFeedLifecycleTest, "Arena.UI.EliminationFeed.LifecycleAndDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaEliminationFeedLifecycleTest::RunTest(const FString& Parameters)
{
	using namespace ArenaEliminationFeedTests;
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

	UClass* FeedClass = LoadClass<UArenaEliminationFeedWidget>(nullptr, FeedPath);
	UArenaEliminationFeedWidget* ProductionFeed = FeedClass ? CreateWidget<UArenaEliminationFeedWidget>(Controller, FeedClass) : nullptr;
	if (!TestNotNull(TEXT("Production feed derives from C++"), ProductionFeed))
	{
		return false;
	}
	TSharedPtr<SWidget> SlateFeed = ProductionFeed->TakeWidget();
	UListView* ProductionList = Cast<UListView>(ProductionFeed->GetWidgetFromName(TEXT("ToastListWidget")));
	UOverlay* VisibilityOverlay = Cast<UOverlay>(ProductionFeed->GetWidgetFromName(TEXT("FeedVisibilityOvr")));
	if (!TestNotNull(TEXT("Production list binds"), ProductionList) || !TestNotNull(TEXT("Visibility presentation binds"), VisibilityOverlay))
	{
		return false;
	}
	TestTrue(TEXT("An empty feed is collapsed by WBP"), VisibilityOverlay->GetVisibility() == ESlateVisibility::Collapsed);

	UArenaEliminationFeedTestWidget* Widget = CreateWidget<UArenaEliminationFeedTestWidget>(Controller);
	Widget->ConfigureForTest(1.0f, 3);
	Widget->ConstructForTest();
	ON_SCOPE_EXIT { Widget->DestructForTest(); };
	UListView* List = Widget->GetListForTest();
	AArenaPlayerState* Attacker = World->SpawnActor<AArenaPlayerState>();
	AArenaPlayerState* Victim = World->SpawnActor<AArenaPlayerState>();
	Attacker->SetPlayerName(TEXT("Attacker"));
	Victim->SetPlayerName(TEXT("Victim"));
	Attacker->SetGenericTeamId(FGenericTeamId(1));
	Victim->SetGenericTeamId(FGenericTeamId(2));
	AddHealth(World->SpawnActor<APawn>(), Attacker);
	AddHealth(World->SpawnActor<APawn>(), Victim);

	UClass* RelayClass = LoadClass<UArenaEliminationFeedRelay>(nullptr, RelayPath);
	UClass* StreakClass = LoadClass<UActorComponent>(nullptr, TEXT("/Game/Blueprints/Accolades/B_ElimStreakProcessor.B_ElimStreakProcessor_C"));
	if (!TestNotNull(TEXT("Load shared production streak processor"), StreakClass))
	{
		return false;
	}
	AActor* StreakOwner = World->SpawnActor<AActor>();
	UActorComponent* Streak = NewObject<UActorComponent>(StreakOwner, StreakClass);
	StreakOwner->AddInstanceComponent(Streak);
	Streak->RegisterComponent();
	if (!TestNotNull(TEXT("Production relay derives from C++"), RelayClass))
	{
		return false;
	}
	AActor* RelayOwner = World->SpawnActor<AActor>();
	UArenaEliminationFeedRelay* Relay = NewObject<UArenaEliminationFeedRelay>(RelayOwner, RelayClass);
	RelayOwner->AddInstanceComponent(Relay);
	Relay->RegisterComponent();
	Relay->StartListening();
	Relay->StartListening();

	Damage(Attacker, Victim, 10.0f);
	TestEqual(TEXT("Nonlethal damage creates no entry"), List->GetNumItems(), 0);
	Damage(Attacker, Victim, 100.0f);
	TestEqual(TEXT("Real lethal GAS damage creates exactly one entry"), List->GetNumItems(), 1);
	TestEqual(TEXT("Production WBP receives real lethal damage"), ProductionList->GetNumItems(), 1);
	TestTrue(TEXT("WBP shows a nonempty feed"), VisibilityOverlay->GetVisibility() == ESlateVisibility::Visible);
	UArenaEliminationFeedItem* First = Cast<UArenaEliminationFeedItem>(List->GetItemAt(0));
	if (!TestNotNull(TEXT("Native item snapshot"), First))
	{
		return false;
	}
	TestEqual(TEXT("Snapshot resolves attacker name"), First->AttackerName.ToString(), FString(TEXT("Attacker")));
	TestEqual(TEXT("Snapshot resolves victim name"), First->VictimName.ToString(), FString(TEXT("Victim")));
	TestEqual(TEXT("Team comes from PlayerState without requiring its pawn"), First->AttackerTeamId, 1);
	TestEqual(TEXT("Victim team is preserved"), First->VictimTeamId, 2);
	TestTrue(TEXT("Enemy classification"), First->EliminationType == EArenaEliminationType::Enemy);
	Damage(Attacker, Victim, 100.0f);
	TestEqual(TEXT("Damage after death cannot duplicate the feed"), List->GetNumItems(), 1);
	Attacker->SetPlayerName(TEXT("Renamed"));
	TestEqual(TEXT("Later name changes cannot rewrite history"), First->AttackerName.ToString(), FString(TEXT("Attacker")));
	TickWorld(World, 0.6f);

	UGameplayMessageSubsystem& Messages = UGameplayMessageSubsystem::Get(World);
	auto Send = [&Messages](const FArenaVerbMessage& Message)
	{
		Messages.BroadcastMessage(Message.Verb, Message);
	};
	Send(MakeMessage(Attacker, Victim, TEXT("Gameplay.Damage.FromAlly")));
	UArenaEliminationFeedItem* Ally = Cast<UArenaEliminationFeedItem>(List->GetItemAt(1));
	TestTrue(TEXT("Friendly elimination classification"), Ally && Ally->EliminationType == EArenaEliminationType::Ally);
	Send(MakeMessage(Victim, Victim));
	UArenaEliminationFeedItem* Self = Cast<UArenaEliminationFeedItem>(List->GetItemAt(2));
	TestTrue(TEXT("Self identity takes priority over enemy tag"), Self && Self->EliminationType == EArenaEliminationType::Self);
	Send(MakeMessage(nullptr, Victim, TEXT("Gameplay.Damage.SelfDestruct")));
	TestEqual(TEXT("Capacity removes oldest entry"), List->GetNumItems(), 3);
	TestFalse(TEXT("Oldest object is removed"), List->GetListItems().Contains(First));
	UArenaEliminationFeedItem* Environment = Cast<UArenaEliminationFeedItem>(List->GetItemAt(2));
	TestTrue(TEXT("Environment death uses a valid fallback"), Environment && !Environment->AttackerName.IsEmpty() && Environment->AttackerTeamId == INDEX_NONE);
	Send(MakeMessage(Attacker, nullptr));
	TestEqual(TEXT("Invalid victim is ignored"), List->GetNumItems(), 3);
	TickWorld(World, 0.5f);
	TestEqual(TEXT("Evicted entry deadline cannot expire newer entries"), List->GetNumItems(), 3);
	TickWorld(World, 0.6f);
	TestEqual(TEXT("Entries expire without instantiated row widgets"), List->GetNumItems(), 0);

	Widget->DestructForTest();
	Send(MakeMessage(Attacker, Victim));
	TestEqual(TEXT("Destruct unregisters listener"), List->GetNumItems(), 0);
	Widget->ConstructForTest();
	Widget->ConstructForTest();
	Send(MakeMessage(Attacker, Victim));
	TestEqual(TEXT("Reconstruction registers only one listener"), List->GetNumItems(), 1);
	Widget->DestructForTest();
	Widget->ConstructForTest();
	TickWorld(World, 1.1f);
	TestEqual(TEXT("Reconstruction clears old entries and deadlines"), List->GetNumItems(), 0);
	Widget->ConfigureForTest(0.0f, 0);
	Send(MakeMessage(Attacker, Victim));
	Send(MakeMessage(Victim, Victim));
	TestEqual(TEXT("Invalid capacity is clamped at runtime"), List->GetNumItems(), 1);
	TickWorld(World, 6.1f);
	TestEqual(TEXT("Invalid lifetime uses finite fallback"), List->GetNumItems(), 0);
	TestEqual(TEXT("Production entries expire"), ProductionList->GetNumItems(), 0);
	TestTrue(TEXT("WBP collapses after final expiration"), VisibilityOverlay->GetVisibility() == ESlateVisibility::Collapsed);

	Widget->DestructForTest();
	Widget->ConfigureForTest(60.0f, 8);
	Widget->ConstructForTest();
	// Reset the attacker through the same production death-message entry point.
	Send(MakeMessage(Victim, Attacker));
	for (int32 Index = 1; Index <= 7; ++Index)
	{
		Send(MakeMessage(Attacker, Victim));
		const UArenaEliminationFeedItem* Latest = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
		TestTrue(TEXT("Shared streak counts enemies and saturates for 6+ presentation"), Latest && Latest->EliminationStreak == FMath::Min(Index, 6));
	}
	AArenaPlayerState* Teammate = World->SpawnActor<AArenaPlayerState>();
	Teammate->SetGenericTeamId(FGenericTeamId(1));
	Send(MakeMessage(Attacker, Teammate, TEXT("Gameplay.Damage.FromAlly")));
	const UArenaEliminationFeedItem* Friendly = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
	TestTrue(TEXT("Friendly fire counts friendly eliminations only"), Friendly && Friendly->EliminationStreak == 1 && Friendly->EliminationType == EArenaEliminationType::Ally);
	Send(MakeMessage(Attacker, Teammate, TEXT("Gameplay.Damage.FromAlly")));
	Friendly = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
	TestTrue(TEXT("Friendly count accumulates without touching the enemy streak"), Friendly && Friendly->EliminationStreak == 2);
	Send(MakeMessage(Attacker, Attacker, TEXT("Gameplay.Damage.SelfDestruct")));
	const UArenaEliminationFeedItem* SelfDeath = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
	TestTrue(TEXT("Self elimination receives its own count"), SelfDeath && SelfDeath->EliminationStreak == 1 && SelfDeath->EliminationType == EArenaEliminationType::Self);
	Send(MakeMessage(nullptr, Victim, TEXT("Gameplay.Damage.SelfDestruct")));
	const UArenaEliminationFeedItem* EnvironmentDeath = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
	TestTrue(TEXT("Environment death has no count"), EnvironmentDeath && EnvironmentDeath->EliminationStreak == 0);
	Send(MakeMessage(Attacker, Victim));
	const UArenaEliminationFeedItem* Restarted = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
	TestTrue(TEXT("Death resets the shared enemy streak"), Restarted && Restarted->EliminationStreak == 1);
	TestTrue(TEXT("Non-grenade eliminations carry no grenade"), Restarted && Restarted->GrenadeDefinition == nullptr);

	// Grenade attribution through the real GAS death path: direct grenade context and grenade-spawned effect actors.
	const UArenaGrenadeDefinitionData* ThrownGrenade = NewObject<UArenaGrenadeDefinitionData>();
	AArenaPlayerState* GrenadeVictim = World->SpawnActor<AArenaPlayerState>();
	GrenadeVictim->SetPlayerName(TEXT("GrenadeVictim"));
	GrenadeVictim->SetGenericTeamId(FGenericTeamId(2));
	AddHealth(World->SpawnActor<APawn>(), GrenadeVictim);
	Damage(Attacker, GrenadeVictim, 100.0f, ThrownGrenade);
	const UArenaEliminationFeedItem* GrenadeKill = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
	TestTrue(TEXT("Grenade damage context credits the thrown grenade"), GrenadeKill && GrenadeKill->VictimName.ToString() == GrenadeVictim->GetPlayerName() && GrenadeKill->GrenadeDefinition == ThrownGrenade);

	AArenaEffectActor* GrenadeEffect = World->SpawnActor<AArenaEffectActor>();
	GrenadeEffect->SetSourceGrenadeDefinition(ThrownGrenade);
	AArenaPlayerState* EffectVictim = World->SpawnActor<AArenaPlayerState>();
	EffectVictim->SetPlayerName(TEXT("EffectVictim"));
	EffectVictim->SetGenericTeamId(FGenericTeamId(2));
	AddHealth(World->SpawnActor<APawn>(), EffectVictim);
	Damage(Attacker, EffectVictim, 100.0f, GrenadeEffect);
	const UArenaEliminationFeedItem* EffectKill = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
	TestTrue(TEXT("Grenade-spawned effect actors credit their grenade"), EffectKill && EffectKill->VictimName.ToString() == EffectVictim->GetPlayerName() && EffectKill->GrenadeDefinition == ThrownGrenade);

	FArenaVerbMessage UnrelatedContext = MakeMessage(Attacker, Victim);
	UnrelatedContext.ContextObject = GrenadeEffect;
	Send(UnrelatedContext);
	const UArenaEliminationFeedItem* UnrelatedKill = Cast<UArenaEliminationFeedItem>(List->GetItemAt(List->GetNumItems() - 1));
	TestTrue(TEXT("Only grenade definitions are exposed as grenade data"), UnrelatedKill && UnrelatedKill->GrenadeDefinition == nullptr);

	auto Clamp = [](const FString& Name, int32 Limit)
	{
		return UArenaEliminationFeedEntry::ClampDisplayName(FText::FromString(Name), Limit).ToString();
	};
	TestEqual(TEXT("Names within the cap are shown in full"), Clamp(TEXT("0123456789ABCDEF"), 16), FString(TEXT("0123456789ABCDEF")));
	TestEqual(TEXT("Long names keep cap - 1 characters plus an ellipsis"), Clamp(TEXT("A_VERY_LONG_PLAYER_NAME_0123456789"), 16), FString(TEXT("A_VERY_LONG_PLA\u2026")));
	TestEqual(TEXT("CJK names are capped by character, not byte"), Clamp(TEXT("\u73A9\u5BB6\u540D\u79F0\u5F88\u957F\u7684\u6D4B\u8BD5"), 5), FString(TEXT("\u73A9\u5BB6\u540D\u79F0\u2026")));
	TestEqual(TEXT("Trailing spaces are trimmed before the ellipsis"), Clamp(TEXT("ABC DEFGH"), 5), FString(TEXT("ABC\u2026")));
	FString Emoji = TEXT("ABC");
	Emoji.AppendChar(0xD83D);
	Emoji.AppendChar(0xDE00);
	Emoji += TEXT("DEF");
	FString ExpectedEmoji = TEXT("ABC");
	ExpectedEmoji.AppendChar(0xD83D);
	ExpectedEmoji.AppendChar(0xDE00);
	ExpectedEmoji += TEXT("\u2026");
	TestEqual(TEXT("Surrogate pairs count as one character and are never split"), Clamp(Emoji, 5), ExpectedEmoji);
	TestEqual(TEXT("Invalid caps fall back to a usable minimum"), Clamp(TEXT("ABCDEF"), 0), FString(TEXT("A\u2026")));
	SlateFeed.Reset();
	return true;
}
#endif
