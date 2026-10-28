// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Messages/GameplayMessageProcessor.h"
#include "ArenaEliminationFeedRelay.generated.h"

struct FArenaVerbMessage;

/** Relays authoritative eliminations through the existing reliable GameState multicast. */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class ARENA_API UArenaEliminationFeedRelay : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	virtual void StartListening() override;
	virtual void StopListening() override;

private:
	void HandleEliminationMessage(FGameplayTag Channel, const FArenaVerbMessage& Message);

	FGameplayMessageListenerHandle ListenerHandle;
};
