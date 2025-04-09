// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Messages/GameplayMessageProcessor.h"

#include "ElimStreakProcessor.generated.h"

class APlayerState;
class UObject;
struct FGameplayTag;
struct FArenaVerbMessage;
template <typename T> struct TObjectPtr;

// Tracks a streak of eliminations (X eliminations without being eliminated)
UCLASS(Abstract)
class UElimStreakProcessor : public UGameplayMessageProcessor
{
	GENERATED_BODY()

public:
	virtual void StartListening() override;

protected:
	// The event to rebroadcast when a user gets a streak of a certain length
	UPROPERTY(EditDefaultsOnly)
	TMap<int32, FGameplayTag> EnemyElimStreakTags;

	UPROPERTY(EditDefaultsOnly)
	TMap<int32, FGameplayTag> SelfElimStreakTags;

	UPROPERTY(EditDefaultsOnly)
	TMap<int32, FGameplayTag> FriendlyElimStreakTags;

private:
	void OnEliminationMessage(FGameplayTag Channel, const FArenaVerbMessage& Payload);

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerState>, int32> PlayerEnemyElimHistory;

	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerState>, int32> PlayerSelfElimHistory;

	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerState>, int32> PlayerFriendlyElimHistory;
};
