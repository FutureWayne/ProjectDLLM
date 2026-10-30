// Copyright Ludens Studio. All Rights Reserved.

#include "Messages/ArenaAccoladeRelay.h"

#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameState/ArenaGameState.h"
#include "Messages/ArenaNotificationMessage.h"
#include "Messages/ArenaVerbMessage.h"
#include "Messages/ArenaVerbMessageHelpers.h"
#include "NativeGameplayTags.h"

namespace ArenaAccoladeRelay
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_Accolade, "Arena.Accolade");
}

void UArenaAccoladeRelay::StartListening()
{
	StopListening();

	TWeakObjectPtr<ThisClass> WeakThis(this);
	ListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener<FArenaVerbMessage>(
		ArenaAccoladeRelay::TAG_Arena_Accolade,
		[WeakThis](FGameplayTag Channel, const FArenaVerbMessage& Message)
		{
			if (ThisClass* Relay = WeakThis.Get())
			{
				Relay->HandleAccoladeMessage(Channel, Message);
			}
		},
		EGameplayMessageMatch::PartialMatch);
}

void UArenaAccoladeRelay::StopListening()
{
	ListenerHandle.Unregister();
}

void UArenaAccoladeRelay::HandleAccoladeMessage(FGameplayTag Channel, const FArenaVerbMessage& Message)
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	if (Owner->HasAuthority())
	{
		// Reliable keeps accolades ordered with the elimination feed on the same GameState channel.
		if (AArenaGameState* GameState = World->GetGameState<AArenaGameState>())
		{
			GameState->MulticastReliableMessageToClients(Message);
		}
	}

	// Clients receive the multicast rebroadcast; listen-server hosts and standalone handle the local message.
	if (World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FArenaNotificationMessage Notification;
	Notification.TargetChannel = ArenaAccoladeRelay::TAG_Arena_Accolade;
	Notification.PayloadTag = Message.Verb;
	Notification.TargetPlayer = UArenaVerbMessageHelpers::GetPlayerStateFromObject(Message.Instigator);
	UGameplayMessageSubsystem::Get(this).BroadcastMessage(TAG_Arena_AddNotification_Message, Notification);
}
