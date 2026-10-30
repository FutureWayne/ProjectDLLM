// Copyright Ludens Studio. All Rights Reserved.

#include "Accolades/ArenaAccoladeHostWidget.h"

#include "DataRegistrySubsystem.h"
#include "ArenaLogChannels.h"
#include "Engine/World.h"
#include "Messages/ArenaNotificationMessage.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaAccoladeHostWidget)

class UUserWidget;

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_Accolade, "Arena.Accolade");

static FName NAME_AccoladeRegistryID("Accolades");

void UArenaAccoladeHostWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResetAccolades();
	bAcceptingNotifications = true;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	ListenerHandle = MessageSubsystem.RegisterListener(TAG_Arena_AddNotification_Message, this, &ThisClass::OnNotificationMessage);
}

void UArenaAccoladeHostWidget::NativeDestruct()
{
	ResetAccolades();
	Super::NativeDestruct();
}

void UArenaAccoladeHostWidget::ResetAccolades()
{
	bAcceptingNotifications = false;
	++LoadGeneration;
	ListenerHandle.Unregister();
	CancelAsyncLoading();
	CancelLatentActions();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NextTimeToReconsiderHandle);
	}

	TArray<FPendingAccoladeEntry> PreviousDisplays = MoveTemp(PendingAccoladeDisplays);
	PendingAccoladeLoads.Reset();
	NextDisplaySequenceID = 0;
	AllocatedSequenceID = 0;
	for (const FPendingAccoladeEntry& Entry : PreviousDisplays)
	{
		if (UUserWidget* Widget = Entry.AllocatedWidget)
		{
			DestroyAccoladeWidget(Widget);
		}
	}
}

void UArenaAccoladeHostWidget::OnNotificationMessage(FGameplayTag Channel, const FArenaNotificationMessage& Notification)
{
	if (bAcceptingNotifications && Notification.TargetChannel == TAG_Arena_Accolade)
	{
		// Every local player sees accolades; the earner's identity scopes cancellation, not delivery.
		if (Notification.TargetPlayer != nullptr)
		{
			APlayerController* PC = GetOwningPlayer();
			if ((PC == nullptr))
			{
				return;
			}
		}

		// Reserve a queue entry before acquiring: registry completion can be synchronous or out of order.
		const int32 NextID = AllocatedSequenceID++;
		FPendingAccoladeEntry& PendingEntry = PendingAccoladeLoads.AddDefaulted_GetRef();
		PendingEntry.SequenceID = NextID;
		PendingEntry.TargetPlayer = Notification.TargetPlayer.Get();
		PendingEntry.TargetPlayerName = Notification.TargetPlayer ? Notification.TargetPlayer->GetPlayerName() : TEXT("");

		FDataRegistryId ItemID(NAME_AccoladeRegistryID, Notification.PayloadTag.GetTagName());
		UDataRegistrySubsystem* Registry = UDataRegistrySubsystem::Get();
		if (!Registry || !Registry->AcquireItem(ItemID, FDataRegistryItemAcquiredCallback::CreateUObject(this, &ThisClass::OnRegistryLoadCompleted, NextID, LoadGeneration)))
		{
			UE_LOG(LogArena, Warning, TEXT("Failed to acquire accolade %s; skipping this notification"), *Notification.PayloadTag.ToString());
			FailPendingAccolade(NextID);
		}
	}
}

void UArenaAccoladeHostWidget::OnRegistryLoadCompleted(const FDataRegistryAcquireResult& AccoladeHandle, int32 SequenceID, uint32 Generation)
{
	if (!bAcceptingNotifications || Generation != LoadGeneration)
	{
		return;
	}
	FPendingAccoladeEntry* PendingEntry = PendingAccoladeLoads.FindByPredicate([SequenceID](const FPendingAccoladeEntry& Entry)
	{
		return Entry.SequenceID == SequenceID;
	});
	if (!PendingEntry || PendingEntry->bFinishedLoading)
	{
		return;
	}

	if (const FArenaAccoladeDefinitionRow* AccoladeRow = AccoladeHandle.GetItem<FArenaAccoladeDefinitionRow>())
	{
		PendingEntry->Row = *AccoladeRow;

		TArray<FSoftObjectPath> AssetsToLoad;
		if (!AccoladeRow->Sound.IsNull())
		{
			AssetsToLoad.Add(AccoladeRow->Sound.ToSoftObjectPath());
		}
		if (!AccoladeRow->Icon.IsNull())
		{
			AssetsToLoad.Add(AccoladeRow->Icon.ToSoftObjectPath());
		}
		auto OnAssetsLoaded = [this, SequenceID, Generation]
		{
			if (!bAcceptingNotifications || Generation != LoadGeneration)
			{
				return;
			}
			FPendingAccoladeEntry* EntryThatFinishedLoading = PendingAccoladeLoads.FindByPredicate([SequenceID](const FPendingAccoladeEntry& Entry) { return Entry.SequenceID == SequenceID; });
			if (EntryThatFinishedLoading && !EntryThatFinishedLoading->bFinishedLoading)
			{
				EntryThatFinishedLoading->Sound = EntryThatFinishedLoading->Row.Sound.Get();
				EntryThatFinishedLoading->Icon = EntryThatFinishedLoading->Row.Icon.Get();
				EntryThatFinishedLoading->bFinishedLoading = true;
				ConsiderLoadedAccolades();
			}
		};
		if (AssetsToLoad.IsEmpty())
		{
			OnAssetsLoaded();
		}
		else
		{
			AsyncLoad(AssetsToLoad, MoveTemp(OnAssetsLoaded));
			StartAsyncLoading();
		}
	}
	else
	{
		UE_LOG(LogArena, Warning, TEXT("Failed to load accolade %s; skipping this notification"), *AccoladeHandle.ItemId.ToString());
		FailPendingAccolade(SequenceID);
	}
}

void UArenaAccoladeHostWidget::FailPendingAccolade(int32 SequenceID)
{
	if (FPendingAccoladeEntry* Entry = PendingAccoladeLoads.FindByPredicate([SequenceID](const FPendingAccoladeEntry& Pending)
	{
		return Pending.SequenceID == SequenceID;
	}))
	{
		Entry->bLoadFailed = true;
		Entry->bFinishedLoading = true;
		ConsiderLoadedAccolades();
	}
}

void UArenaAccoladeHostWidget::ConsiderLoadedAccolades()
{
	int32 PendingIndexToDisplay;
	do
	{
		PendingIndexToDisplay = PendingAccoladeLoads.IndexOfByPredicate([DesiredID=NextDisplaySequenceID](const FPendingAccoladeEntry& Entry) { return Entry.bFinishedLoading && Entry.SequenceID == DesiredID; });
		if (PendingIndexToDisplay != INDEX_NONE)
		{
			FPendingAccoladeEntry Entry = MoveTemp(PendingAccoladeLoads[PendingIndexToDisplay]);
			PendingAccoladeLoads.RemoveAtSwap(PendingIndexToDisplay);

			++NextDisplaySequenceID;
			if (!Entry.bLoadFailed)
			{
				ProcessLoadedAccolade(Entry);
			}
		}
	} while (PendingIndexToDisplay != INDEX_NONE);
}

void UArenaAccoladeHostWidget::ProcessLoadedAccolade(const FPendingAccoladeEntry& Entry)
{
	if (Entry.Row.LocationTag == LocationName)
	{
		bool bRecreateWidget = PendingAccoladeDisplays.Num() == 0;
		for (int32 Index = 0; Index < PendingAccoladeDisplays.Num(); )
		{
			if (PendingAccoladeDisplays[Index].TargetPlayer.HasSameIndexAndSerialNumber(Entry.TargetPlayer) &&
				PendingAccoladeDisplays[Index].Row.AccoladeTags.HasAny(Entry.Row.CancelAccoladesWithTag))
			{
				bRecreateWidget |= Index == 0;
				if (UUserWidget* OldWidget = PendingAccoladeDisplays[Index].AllocatedWidget)
				{
					DestroyAccoladeWidget(OldWidget);
					bRecreateWidget = true;
				}
				PendingAccoladeDisplays.RemoveAt(Index);
			}
			else
			{
				++Index;
			}
		}

		PendingAccoladeDisplays.Add(Entry);
		
		if (bRecreateWidget)
		{
			DisplayNextAccolade();
		}
	}
}

void UArenaAccoladeHostWidget::DisplayNextAccolade()
{
	if (PendingAccoladeDisplays.Num() > 0)
	{
		FPendingAccoladeEntry& Entry = PendingAccoladeDisplays[0];

		GetWorld()->GetTimerManager().SetTimer(NextTimeToReconsiderHandle, this, &ThisClass::PopDisplayedAccolade, Entry.Row.DisplayDuration);
		Entry.AllocatedWidget = CreateAccoladeWidget(Entry);
	}
}

void UArenaAccoladeHostWidget::PopDisplayedAccolade()
{
	if (PendingAccoladeDisplays.Num() > 0)
	{
		if (UUserWidget* OldWidget = PendingAccoladeDisplays[0].AllocatedWidget)
		{
			DestroyAccoladeWidget(OldWidget);
		}
		PendingAccoladeDisplays.RemoveAt(0);
	}

	DisplayNextAccolade();
}

