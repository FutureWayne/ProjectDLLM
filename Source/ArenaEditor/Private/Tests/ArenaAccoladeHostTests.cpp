// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaAccoladeHostTestTypes.h"

EDataRegistryAvailability UArenaAccoladeTestTableSource::GetItemAvailability(const FName& ResolvedName, const uint8** PrecachedDataPtr) const
{
	// Advertise an uncached row, then let the real table lookup report its absence asynchronously.
	if (ResolvedName == TEXT("Arena.Accolade.FriendlyKillStreak.30"))
	{
		return EDataRegistryAvailability::Unknown;
	}
	return Super::GetItemAvailability(ResolvedName, PrecachedDataPtr);
}

void UArenaAccoladeTestTableSource::HandlePendingAcquires()
{
	// Tests release actual source requests explicitly to exercise out-of-order and late completions.
}

bool UArenaAccoladeTestTableSource::CompleteRequest(FName RowName)
{
	const int32 Index = PendingAcquires.IndexOfByPredicate([RowName](const FDataRegistrySourceAcquireRequest& Request)
	{
		uint8 SourceIndex = 255;
		FName ResolvedName;
		return Request.Lookup.GetEntry(SourceIndex, ResolvedName, Request.LookupIndex) && ResolvedName == RowName;
	});
	if (Index == INDEX_NONE)
	{
		return false;
	}
	const FDataRegistrySourceAcquireRequest Request = PendingAcquires[Index];
	PendingAcquires.RemoveAt(Index);
	TArray<FDataRegistrySourceAcquireRequest> Remaining = MoveTemp(PendingAcquires);
	PendingAcquires.Add(Request);
	Super::HandlePendingAcquires();
	PendingAcquires.Append(MoveTemp(Remaining));
	return true;
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Accolades/ArenaAccoladeHostWidget.h"
#include "Components/Widget.h"
#include "DataRegistrySubsystem.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/LatentActionManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "LatentActions.h"
#include "Messages/ArenaNotificationMessage.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

/** Observes queue state while notifications, registry acquisition and WBP presentation use production paths. */
class FArenaAccoladeHostTestAccess
{
public:
	static int32 Loading(const UArenaAccoladeHostWidget& Host) { return Host.PendingAccoladeLoads.Num(); }
	static int32 Displaying(const UArenaAccoladeHostWidget& Host) { return Host.PendingAccoladeDisplays.Num(); }
	static int32 NextSequence(const UArenaAccoladeHostWidget& Host) { return Host.NextDisplaySequenceID; }
	static int32 AllocatedSequence(const UArenaAccoladeHostWidget& Host) { return Host.AllocatedSequenceID; }
	static bool HasDisplayTimer(const UArenaAccoladeHostWidget& Host)
	{
		return Host.GetWorld()->GetTimerManager().TimerExists(Host.NextTimeToReconsiderHandle);
	}
	static void FinishAssetLoads(UArenaAccoladeHostWidget& Host)
	{
		FlushAsyncLoading();
		Host.StartAsyncLoading();
	}
	static const FPendingAccoladeEntry& Entry(const UArenaAccoladeHostWidget& Host, int32 Index)
	{
		static const FPendingAccoladeEntry EmptyEntry;
		return Host.PendingAccoladeDisplays.IsValidIndex(Index) ? Host.PendingAccoladeDisplays[Index] : EmptyEntry;
	}
};

namespace ArenaAccoladeHostTests
{
	const FName DoubleRow(TEXT("Arena.Accolade.EliminationStreak.2"));
	const FName TripleRow(TEXT("Arena.Accolade.EliminationStreak.3"));
	const FName QuadraRow(TEXT("Arena.Accolade.EliminationStreak.4"));
	const FName PentaRow(TEXT("Arena.Accolade.EliminationStreak.5"));
	const FName MissingRow(TEXT("Arena.Accolade.FriendlyKillStreak.30"));

	void Send(UArenaAccoladeHostWidget* Host, APlayerState* Player, FName Row)
	{
		FArenaNotificationMessage Notification;
		Notification.TargetChannel = FGameplayTag::RequestGameplayTag(TEXT("Arena.Accolade"));
		Notification.PayloadTag = FGameplayTag::RequestGameplayTag(Row);
		Notification.TargetPlayer = Player;
		UGameplayMessageSubsystem::Get(Host).BroadcastMessage(TAG_Arena_AddNotification_Message, Notification);
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

	class FCountingLatentAction : public FPendingLatentAction
	{
	public:
		explicit FCountingLatentAction(int32& InExecutions) : Executions(InExecutions) {}
		virtual void UpdateOperation(FLatentResponse& Response) override
		{
			++Executions;
			Response.DoneIf(true);
		}
	private:
		int32& Executions;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaAccoladeHostQueueTest, "Arena.UI.Accolades.QueueAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaAccoladeHostQueueTest::RunTest(const FString& Parameters)
{
	using namespace ArenaAccoladeHostTests;
	using Access = FArenaAccoladeHostTestAccess;
	AddExpectedMessage(TEXT("Failed to load accolade"), ELogVerbosity::Warning);
	AddExpectedMessage(TEXT("Failed to acquire accolade"), ELogVerbosity::Warning);
	UDataRegistrySubsystem* Subsystem = UDataRegistrySubsystem::Get();
	UDataRegistry* Registry = Subsystem ? Subsystem->GetRegistryForType(TEXT("Accolades")) : nullptr;
	UDataTable* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/Blueprints/Accolades/DT_Accolades.DT_Accolades"));
	UClass* HostClass = LoadClass<UArenaAccoladeHostWidget>(nullptr,
		TEXT("/Game/UI/Notifications/Accolades/W_AccoladeHostWidget.W_AccoladeHostWidget_C"));
	if (!TestNotNull(TEXT("Production accolade registry"), Registry) || !TestNotNull(TEXT("Production accolade table"), Table) ||
		!TestNotNull(TEXT("Production accolade host WBP"), HostClass))
	{
		return false;
	}

	// Preload the production presentation resources; only table completion order is controlled by this test.
	for (FName RowName : { DoubleRow, TripleRow, QuadraRow, PentaRow })
	{
		const FArenaAccoladeDefinitionRow* Row = reinterpret_cast<const FArenaAccoladeDefinitionRow*>(Table->FindRowUnchecked(RowName));
		if (!TestNotNull(TEXT("Production accolade row"), Row))
		{
			return false;
		}
		Row->Sound.LoadSynchronous();
		Row->Icon.LoadSynchronous();
	}

	const FArrayProperty* SourcesProperty = FindFProperty<FArrayProperty>(Registry->GetClass(), TEXT("DataSources"));
	if (!TestNotNull(TEXT("Registry source configuration"), SourcesProperty))
	{
		return false;
	}
	TArray<TObjectPtr<UDataRegistrySource>>& Sources = *SourcesProperty->ContainerPtrToValuePtr<TArray<TObjectPtr<UDataRegistrySource>>>(Registry);
	const TArray<TObjectPtr<UDataRegistrySource>> OriginalSources = Sources;
	TArray<TStrongObjectPtr<UDataRegistrySource>> KeepOriginalSources;
	for (UDataRegistrySource* Source : OriginalSources)
	{
		KeepOriginalSources.Emplace(Source);
	}
	Registry->Deinitialize();
	UArenaAccoladeTestTableSource* Source = NewObject<UArenaAccoladeTestTableSource>(Registry);
	Source->SetSourceTable(Table, FDataRegistrySource_DataTableRules());
	Sources = { Source };
	Registry->Initialize();
	ON_SCOPE_EXIT
	{
		Registry->Deinitialize();
		Sources = OriginalSources;
		Registry->Initialize();
	};

	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
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
	UArenaAccoladeHostWidget* Host = CreateWidget<UArenaAccoladeHostWidget>(Controller, HostClass);
	TSharedPtr<SWidget> SlateHost = Host->TakeWidget();
	ON_SCOPE_EXIT
	{
		SlateHost.Reset();
		Host->NativeDestruct();
	};
	APlayerState* PlayerA = World->SpawnActor<APlayerState>();
	APlayerState* PlayerB = World->SpawnActor<APlayerState>();
	PlayerA->SetPlayerName(TEXT("SameName"));
	PlayerB->SetPlayerName(TEXT("SameName"));

	// The later valid row completes first, but an earlier missing row must release its sequence slot.
	Send(Host, PlayerA, MissingRow);
	Send(Host, PlayerB, DoubleRow);
	TestEqual(TEXT("Both requests use the real delayed table source"), Source->GetPendingCount(), 2);
	TestTrue(TEXT("Resolve later table request first"), Source->CompleteRequest(DoubleRow));
	Access::FinishAssetLoads(*Host);
	TestEqual(TEXT("Later notification waits for its predecessor"), Access::Displaying(*Host), 0);
	TestTrue(TEXT("Real table lookup reports the missing row"), Source->CompleteRequest(MissingRow));
	Access::FinishAssetLoads(*Host);
	TestEqual(TEXT("Async failure releases the next valid notification"), Access::Displaying(*Host), 1);
	TestEqual(TEXT("Failed slot and successful slot both advance"), Access::NextSequence(*Host), 2);
	TestEqual(TEXT("No blocked loading entries remain"), Access::Loading(*Host), 0);
	Send(Host, PlayerA, TEXT("Arena.Accolade.FriendlyKillStreak"));
	TestEqual(TEXT("Immediate lookup failure also advances"), Access::NextSequence(*Host), 3);
	Send(Host, PlayerA, TripleRow);
	TestTrue(TEXT("Valid request after immediate failure"), Source->CompleteRequest(TripleRow));
	Access::FinishAssetLoads(*Host);
	TestEqual(TEXT("Different players with identical names do not cancel each other"), Access::Displaying(*Host), 2);
	TestTrue(TEXT("First player remains current"), Access::Entry(*Host, 0).TargetPlayer.Get() == PlayerB);
	TestNotNull(TEXT("Production WBP creates the visible toast"), Access::Entry(*Host, 0).AllocatedWidget.Get());
	TestTrue(TEXT("Visible entry owns an expiration timer"), Access::HasDisplayTimer(*Host));

	// A newer entry replaces only that player's queued entry, without restarting another player's toast.
	Send(Host, PlayerA, QuadraRow);
	TestTrue(TEXT("Resolve same-player upgrade"), Source->CompleteRequest(QuadraRow));
	Access::FinishAssetLoads(*Host);
	TestEqual(TEXT("Same-player queued upgrade replaces exactly one entry"), Access::Displaying(*Host), 2);
	TestEqual(TEXT("Queued upgrade is the real quadra row"), Access::Entry(*Host, 1).Row.DisplayName.ToString(), FString(TEXT("Quadra Kill")));
	Send(Host, PlayerB, PentaRow);
	TestTrue(TEXT("Resolve current-player upgrade"), Source->CompleteRequest(PentaRow));
	Access::FinishAssetLoads(*Host);
	TestEqual(TEXT("Replacing current player keeps the other player's entry"), Access::Displaying(*Host), 2);
	TestTrue(TEXT("Other player's queued toast becomes current"), Access::Entry(*Host, 0).TargetPlayer.Get() == PlayerA);
	TestNotNull(TEXT("Next toast is created immediately"), Access::Entry(*Host, 0).AllocatedWidget.Get());
	TickWorld(World, 4.1f);
	TestEqual(TEXT("Real timer advances to remaining player's toast"), Access::Displaying(*Host), 1);
	TestTrue(TEXT("Remaining toast belongs to PlayerB"), Access::Entry(*Host, 0).TargetPlayer.Get() == PlayerB);

	// Unspecified-player entries must not cancel a departed player whose weak reference now resolves to null.
	Host->NativeDestruct();
	Host->NativeConstruct();
	Registry->ResetRuntimeState();
	APlayerState* DepartingPlayer = World->SpawnActor<APlayerState>();
	const TWeakObjectPtr<APlayerState> DepartingIdentity(DepartingPlayer);
	Send(Host, DepartingPlayer, DoubleRow);
	TestTrue(TEXT("Resolve departing player's toast"), Source->CompleteRequest(DoubleRow));
	Access::FinishAssetLoads(*Host);
	Send(Host, nullptr, TripleRow);
	TestTrue(TEXT("Resolve unspecified-player toast"), Source->CompleteRequest(TripleRow));
	Access::FinishAssetLoads(*Host);
	DepartingPlayer->Destroy();
	TestFalse(TEXT("Departed player weak reference is invalid"), DepartingIdentity.IsValid());
	Send(Host, nullptr, QuadraRow);
	TestTrue(TEXT("Resolve unspecified-player upgrade"), Source->CompleteRequest(QuadraRow));
	Access::FinishAssetLoads(*Host);
	TestEqual(TEXT("Unspecified-player upgrade cancels only the unspecified predecessor"), Access::Displaying(*Host), 2);
	TestTrue(TEXT("Departed player's identity remains distinct from an unspecified player"),
		Access::Entry(*Host, 0).TargetPlayer.HasSameIndexAndSerialNumber(DepartingIdentity));

	// Keep a real source callback pending across destruction and sequence reuse.
	Registry->ResetRuntimeState();
	Send(Host, PlayerA, DoubleRow);
	TestEqual(TEXT("Outstanding request before destruction"), Access::Loading(*Host), 1);
	int32 LatentExecutions = 0;
	World->GetLatentActionManager().AddNewAction(Host, 42, new FCountingLatentAction(LatentExecutions));
	Host->NativeDestruct();
	TestEqual(TEXT("Destruct clears displayed entries"), Access::Displaying(*Host), 0);
	TestEqual(TEXT("Destruct clears pending loads"), Access::Loading(*Host), 0);
	TestFalse(TEXT("Destruct clears display timer"), Access::HasDisplayTimer(*Host));
	TestEqual(TEXT("Destruct resets allocated sequence"), Access::AllocatedSequence(*Host), 0);
	TestEqual(TEXT("Destruct resets display sequence"), Access::NextSequence(*Host), 0);
	Send(Host, PlayerB, TripleRow);
	TestEqual(TEXT("Destruct unregisters message listener"), Access::Loading(*Host), 0);
	TickWorld(World, 0.02f);
	TestEqual(TEXT("Widget latent work is cancelled"), LatentExecutions, 0);
	Host->NativeConstruct();
	Host->NativeConstruct();
	Send(Host, PlayerB, TripleRow);
	TestEqual(TEXT("Reconstructed host has exactly one listener"), Access::Loading(*Host), 1);
	TestTrue(TEXT("Complete old generation's actual registry request"), Source->CompleteRequest(DoubleRow));
	Access::FinishAssetLoads(*Host);
	TestEqual(TEXT("Old callback cannot consume reused sequence zero"), Access::Displaying(*Host), 0);
	TestEqual(TEXT("New generation's request stays pending"), Access::Loading(*Host), 1);
	TestTrue(TEXT("Resolve reconstructed host's request"), Source->CompleteRequest(TripleRow));
	Access::FinishAssetLoads(*Host);
	TestEqual(TEXT("Reconstruction resumes normal presentation"), Access::Displaying(*Host), 1);
	TestTrue(TEXT("Reconstructed entry has the new player's identity"), Access::Entry(*Host, 0).TargetPlayer.Get() == PlayerB);
	Send(Host, PlayerA, QuadraRow);
	Host->NativeDestruct();
	TestTrue(TEXT("Complete real callback while host remains destructed"), Source->CompleteRequest(QuadraRow));
	Access::FinishAssetLoads(*Host);
	TickWorld(World, 4.1f);
	TestEqual(TEXT("Late callbacks and old timers cannot repopulate a destructed host"), Access::Displaying(*Host), 0);
	TestEqual(TEXT("Destructed host has no pending loads"), Access::Loading(*Host), 0);
	return true;
}
#endif
