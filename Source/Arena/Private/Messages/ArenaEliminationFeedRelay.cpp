// Copyright Ludens Studio. All Rights Reserved.

#include "Messages/ArenaEliminationFeedRelay.h"

#include "Engine/World.h"
#include "GameState/ArenaGameState.h"
#include "Messages/ArenaVerbMessage.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_EliminationFeed_Message, "Arena.Elimination.Feed");

void UArenaEliminationFeedRelay::StartListening()
{
	StopListening();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ListenerHandle = UGameplayMessageSubsystem::Get(this).RegisterListener(
			TAG_Arena_EliminationFeed_Message, this, &ThisClass::HandleEliminationMessage);
	}
}

void UArenaEliminationFeedRelay::StopListening()
{
	ListenerHandle.Unregister();
}

void UArenaEliminationFeedRelay::HandleEliminationMessage(FGameplayTag Channel, const FArenaVerbMessage& Message)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (AArenaGameState* GameState = GetWorld()->GetGameState<AArenaGameState>())
		{
			GameState->MulticastReliableMessageToClients(Message);
		}
	}
}
