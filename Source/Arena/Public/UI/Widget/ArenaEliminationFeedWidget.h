// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "TimerManager.h"
#include "ArenaEliminationFeedWidget.generated.h"

class UListView;
class UArenaEliminationFeedItem;
struct FArenaVerbMessage;

/** Owns local elimination snapshots and expiration independently of recycled entry widgets. */
UCLASS(Abstract, Blueprintable)
class ARENA_API UArenaEliminationFeedWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|EliminationFeed|Presentation")
	void OnFeedEntriesChanged(bool bHasEntries);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UListView> ToastListWidget;

	/** Lifetime in game seconds, matching the WBP's timed outro. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|EliminationFeed", meta = (ClampMin = "0.01"))
	float EntryLifetime = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|EliminationFeed", meta = (ClampMin = "1"))
	int32 MaxEntries = 8;

	/** Restricts personal confirmations to eliminations attributed to this widget's local player. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|EliminationFeed")
	bool bOnlyOwningPlayerEliminations = false;

private:
	void HandleEliminationMessage(FGameplayTag Channel, const FArenaVerbMessage& Message);
	void ExpireEntries();
	void ScheduleExpiration();
	void ClearFeed();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArenaEliminationFeedItem>> Entries;

	FGameplayMessageListenerHandle ListenerHandle;
	FTimerHandle ExpirationTimer;
};
