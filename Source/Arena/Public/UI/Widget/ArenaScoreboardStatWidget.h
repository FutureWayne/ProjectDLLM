// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ArenaScoreboardStatWidget.generated.h"

class UAsyncAction_ListenForGameplayMessage;
class UScriptStruct;
class UWorld;

/** Owns one score subscription for the lifetime of a displayed scoreboard stat. */
UCLASS()
class ARENA_API UArenaScoreboardStatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Arena|Scoreboard")
	void EnsureScoreListener();

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Scoreboard")
	FGameplayTag ScoreMessageChannel;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Scoreboard")
	TObjectPtr<UScriptStruct> ScoreMessagePayloadType = nullptr;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Scoreboard")
	void OnScoreChanged();

private:
	void ReleaseScoreListener();
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	UFUNCTION()
	void HandleScoreMessage(UAsyncAction_ListenForGameplayMessage* ProxyObject, FGameplayTag Channel);

	UPROPERTY(Transient)
	TObjectPtr<UAsyncAction_ListenForGameplayMessage> ScoreListener = nullptr;

	TWeakObjectPtr<UWorld> ScoreWorld;
	FDelegateHandle WorldCleanupHandle;

	bool bIsConstructed = false;
	bool bWantsScoreListener = false;
};
