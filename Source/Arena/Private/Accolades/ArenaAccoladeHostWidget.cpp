// Copyright Ludens Studio. All Rights Reserved.

#include "Accolades/ArenaAccoladeHostWidget.h"

#include "DataRegistrySubsystem.h"
#include "ArenaLogChannels.h"
#include "Messages/ArenaNotificationMessage.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaAccoladeHostWidget)

class UUserWidget;

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_Accolade, "Arena.Accolade");

static FName NAME_AccoladeRegistryID("Accolades");

void UArenaAccoladeHostWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	ListenerHandle = MessageSubsystem.RegisterListener(TAG_Arena_AddNotification_Message, this, &ThisClass::OnNotificationMessage);
}

void UArenaAccoladeHostWidget::NativeDestruct()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.UnregisterListener(ListenerHandle);

	CancelAsyncLoading();

	Super::NativeDestruct();
}

void UArenaAccoladeHostWidget::OnNotificationMessage(FGameplayTag Channel, const FArenaNotificationMessage& Notification)
{
	if (Notification.TargetChannel == TAG_Arena_Accolade)
	{
		// Ignore notifications for other players
		if (Notification.TargetPlayer != nullptr)
		{
			APlayerController* PC = GetOwningPlayer();
			if ((PC == nullptr) || (PC->PlayerState != Notification.TargetPlayer))
			{
				return;
			}
		}

		// Load the data registry row for this accolade
		const int32 NextID = AllocatedSequenceID;
		++AllocatedSequenceID;

		FDataRegistryId ItemID(NAME_AccoladeRegistryID, Notification.PayloadTag.GetTagName());
		if (!UDataRegistrySubsystem::Get()->AcquireItem(ItemID, FDataRegistryItemAcquiredCallback::CreateUObject(this, &ThisClass::OnRegistryLoadCompleted, NextID)))
		{
			UE_LOG(LogArena, Error, TEXT("Failed to find accolade registry for tag %s, accolades will not appear"), *Notification.PayloadTag.GetTagName().ToString());
			--AllocatedSequenceID;
		}
	}
}

void UArenaAccoladeHostWidget::OnRegistryLoadCompleted(const FDataRegistryAcquireResult& AccoladeHandle, int32 SequenceID)
{
	if (const FArenaAccoladeDefinitionRow* AccoladeRow = AccoladeHandle.GetItem<FArenaAccoladeDefinitionRow>())
	{
		FPendingAccoladeEntry& PendingEntry = PendingAccoladeLoads.AddDefaulted_GetRef();
		PendingEntry.Row = *AccoladeRow;
		PendingEntry.SequenceID = SequenceID;

		TArray<FSoftObjectPath> AssetsToLoad;
		AssetsToLoad.Add(AccoladeRow->Sound.ToSoftObjectPath());
		AssetsToLoad.Add(AccoladeRow->Icon.ToSoftObjectPath());
		AsyncLoad(AssetsToLoad, [this, SequenceID]
		{
			FPendingAccoladeEntry* EntryThatFinishedLoading = PendingAccoladeLoads.FindByPredicate([SequenceID](const FPendingAccoladeEntry& Entry) { return Entry.SequenceID == SequenceID; });
			if (ensure(EntryThatFinishedLoading))
			{
				EntryThatFinishedLoading->Sound = EntryThatFinishedLoading->Row.Sound.Get();
				EntryThatFinishedLoading->Icon = EntryThatFinishedLoading->Row.Icon.Get();
				EntryThatFinishedLoading->bFinishedLoading = true;
				ConsiderLoadedAccolades();
			}
		});
		StartAsyncLoading();
	}
	else
	{
		ensure(false);
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

			ProcessLoadedAccolade(Entry);
			++NextDisplaySequenceID;
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
			if (PendingAccoladeDisplays[Index].Row.AccoladeTags.HasAny(Entry.Row.CancelAccoladesWithTag))
			{
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

