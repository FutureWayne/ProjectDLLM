// Copyright Ludens Studio. All Rights Reserved.

#include "UI/Widget/ArenaEliminationFeedWidget.h"

#include "Components/ListView.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Messages/ArenaVerbMessage.h"
#include "Messages/ArenaVerbMessageHelpers.h"
#include "NativeGameplayTags.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "UI/ArenaEliminationFeedEntry.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

#define LOCTEXT_NAMESPACE "ArenaEliminationFeed"

namespace ArenaEliminationFeed
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Elimination, "Arena.Elimination.Feed");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SelfDestruct, "Gameplay.Damage.SelfDestruct");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_FromAlly, "Gameplay.Damage.FromAlly");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_FromEnemy, "Gameplay.Damage.FromEnemy");

	// Guards the two-digit count slot against malformed magnitudes; the processor saturates earlier.
	constexpr int32 MaxDisplayedCount = 99;
}

void UArenaEliminationFeedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ListenerHandle.Unregister();
	ClearFeed();
	if (GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		ListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(
			ArenaEliminationFeed::TAG_Elimination, this, &ThisClass::HandleEliminationMessage);
	}
}

void UArenaEliminationFeedWidget::NativeDestruct()
{
	ListenerHandle.Unregister();
	ClearFeed();
	Super::NativeDestruct();
}

void UArenaEliminationFeedWidget::HandleEliminationMessage(FGameplayTag Channel, const FArenaVerbMessage& Message)
{
	APlayerState* Victim = UArenaVerbMessageHelpers::GetPlayerStateFromObject(Message.Target);
	if (!IsValid(Victim) || !ToastListWidget || Message.Verb != ArenaEliminationFeed::TAG_Elimination)
	{
		return;
	}

	APlayerState* Attacker = UArenaVerbMessageHelpers::GetPlayerStateFromObject(Message.Instigator);
	if (bOnlyOwningPlayerEliminations)
	{
		const APlayerState* LocalPlayerState = GetOwningPlayerState();
		if (!IsValid(LocalPlayerState) || !IsValid(Attacker) || Attacker != LocalPlayerState)
		{
			return;
		}
	}

	UArenaEliminationFeedItem* Item = NewObject<UArenaEliminationFeedItem>(this);
	Item->AttackerName = IsValid(Attacker) ? FText::FromString(Attacker->GetPlayerName()) : LOCTEXT("Environment", "Environment");
	Item->VictimName = FText::FromString(Victim->GetPlayerName());
	if (const UArenaTeamSubsystem* Teams = GetWorld()->GetSubsystem<UArenaTeamSubsystem>())
	{
		Item->AttackerTeamId = Teams->FindTeamFromObject(Attacker);
		Item->VictimTeamId = Teams->FindTeamFromObject(Victim);
	}

	if (Attacker == Victim || Message.ContextTags.HasTag(ArenaEliminationFeed::TAG_SelfDestruct))
	{
		Item->EliminationType = EArenaEliminationType::Self;
	}
	else if (Message.ContextTags.HasTag(ArenaEliminationFeed::TAG_FromAlly))
	{
		Item->EliminationType = EArenaEliminationType::Ally;
	}
	else if (Message.ContextTags.HasTag(ArenaEliminationFeed::TAG_FromEnemy))
	{
		Item->EliminationType = EArenaEliminationType::Enemy;
	}
	if (FMath::IsFinite(Message.Magnitude))
	{
		const double MaxCount = static_cast<double>(ArenaEliminationFeed::MaxDisplayedCount);
		Item->EliminationStreak = static_cast<int32>(FMath::Clamp(Message.Magnitude, 0.0, MaxCount));
	}
	Item->GrenadeDefinition = Cast<UArenaGrenadeDefinitionData>(Message.ContextObject.Get());

	const float Lifetime = FMath::IsFinite(EntryLifetime) && EntryLifetime > 0.0f ? EntryLifetime : 6.0f;
	Item->ExpirationTime = GetWorld()->GetTimeSeconds() + Lifetime;
	Entries.Add(Item);
	ToastListWidget->AddItem(Item);
	while (Entries.Num() > FMath::Max(1, MaxEntries))
	{
		ToastListWidget->RemoveItem(Entries[0]);
		Entries.RemoveAt(0);
	}
	OnFeedEntriesChanged(true);
	ScheduleExpiration();
}

void UArenaEliminationFeedWidget::ExpireEntries()
{
	const double Now = GetWorld()->GetTimeSeconds();
	for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
	{
		if (Entries[Index]->ExpirationTime <= Now)
		{
			ToastListWidget->RemoveItem(Entries[Index]);
			Entries.RemoveAt(Index);
		}
	}
	OnFeedEntriesChanged(!Entries.IsEmpty());
	ScheduleExpiration();
}

void UArenaEliminationFeedWidget::ScheduleExpiration()
{
	FTimerManager& Timers = GetWorld()->GetTimerManager();
	Timers.ClearTimer(ExpirationTimer);
	if (!Entries.IsEmpty())
	{
		double NextExpiration = Entries[0]->ExpirationTime;
		for (const UArenaEliminationFeedItem* Item : Entries)
		{
			NextExpiration = FMath::Min(NextExpiration, Item->ExpirationTime);
		}
		const float Delay = FMath::Max(0.001f, static_cast<float>(NextExpiration - GetWorld()->GetTimeSeconds()));
		Timers.SetTimer(ExpirationTimer, this, &ThisClass::ExpireEntries, Delay, false);
	}
}

void UArenaEliminationFeedWidget::ClearFeed()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExpirationTimer);
	}
	Entries.Reset();
	if (ToastListWidget)
	{
		ToastListWidget->ClearListItems();
	}
	OnFeedEntriesChanged(false);
}

#undef LOCTEXT_NAMESPACE
