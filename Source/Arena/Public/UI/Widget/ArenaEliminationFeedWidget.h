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

	/**
	 * With bOnlyOwningPlayerEliminations, also accepts the local player being eliminated by another player,
	 * so the same widget can name the killer. Environment deaths have no killer and stay filtered.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|EliminationFeed")
	bool bIncludeOwningPlayerDeaths = false;

	/** Inserts new entries at the top of the list so the latest confirmation stays closest to the reticle. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|EliminationFeed")
	bool bNewestFirst = false;

	/**
	 * Row height plus list spacing, in slate units. When positive and bNewestFirst is false, entries are
	 * bottom-aligned within MaxEntries rows and the stack glides up one row whenever a new entry arrives.
	 * Requires the list to be at least MaxEntries rows tall so no entry scrolls out of view.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|EliminationFeed", meta = (ClampMin = "0"))
	float StackRiseRowPitch = 0.0f;

	/** Interpolation speed of the rise; higher values settle faster. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|EliminationFeed", meta = (ClampMin = "1"))
	float StackRiseSpeed = 14.0f;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void HandleEliminationMessage(FGameplayTag Channel, const FArenaVerbMessage& Message);
	void ExpireEntries();
	void ScheduleExpiration();
	void ClearFeed();

	bool IsStackRiseEnabled() const;
	float GetStackRiseTarget() const;
	/** Keeps surviving entries visually still after Count entries leave the head of the list. */
	void CompensateStackForRemovedHead(int32 Count);
	void ApplyStackOffset();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArenaEliminationFeedItem>> Entries;

	FGameplayMessageListenerHandle ListenerHandle;
	FTimerHandle ExpirationTimer;
	float StackOffset = 0.0f;
};
