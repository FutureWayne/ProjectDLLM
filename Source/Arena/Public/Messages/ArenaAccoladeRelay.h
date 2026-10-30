// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Messages/GameplayMessageProcessor.h"
#include "ArenaAccoladeRelay.generated.h"

struct FArenaVerbMessage;

/**
 * Shows every accolade to every player. The server relays authoritative Arena.Accolade.* messages through
 * the reliable GameState multicast; each non-dedicated machine turns them into accolade notifications.
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class ARENA_API UArenaAccoladeRelay : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	virtual void StartListening() override;
	virtual void StopListening() override;

private:
	void HandleAccoladeMessage(FGameplayTag Channel, const FArenaVerbMessage& Message);

	FGameplayMessageListenerHandle ListenerHandle;
};
