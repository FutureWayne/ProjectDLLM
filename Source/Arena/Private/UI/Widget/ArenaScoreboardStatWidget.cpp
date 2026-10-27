// Copyright Ludens Studio. All Rights Reserved.

#include "UI/Widget/ArenaScoreboardStatWidget.h"

#include "GameFramework/AsyncAction_ListenForGameplayMessage.h"
#include "Engine/World.h"

void UArenaScoreboardStatWidget::EnsureScoreListener()
{
	bWantsScoreListener = true;
	if (!bIsConstructed || IsDesignTime() || ScoreListener || !ScoreMessageChannel.IsValid() || !ScoreMessagePayloadType)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}

	ScoreListener = UAsyncAction_ListenForGameplayMessage::ListenForGameplayMessages(
		this, ScoreMessageChannel, ScoreMessagePayloadType);
	if (ScoreListener)
	{
		ScoreListener->OnMessageReceived.AddDynamic(this, &ThisClass::HandleScoreMessage);
		ScoreWorld = World;
		WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::HandleWorldCleanup);
		ScoreListener->Activate();
	}
}

void UArenaScoreboardStatWidget::NativeConstruct()
{
	bIsConstructed = true;
	Super::NativeConstruct();
	if (bWantsScoreListener)
	{
		EnsureScoreListener();
	}
}

void UArenaScoreboardStatWidget::NativeDestruct()
{
	bIsConstructed = false;
	ReleaseScoreListener();
	Super::NativeDestruct();
}

void UArenaScoreboardStatWidget::BeginDestroy()
{
	ReleaseScoreListener();
	Super::BeginDestroy();
}

void UArenaScoreboardStatWidget::ReleaseScoreListener()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	WorldCleanupHandle.Reset();
	ScoreWorld.Reset();
	if (ScoreListener)
	{
		ScoreListener->OnMessageReceived.RemoveAll(this);
		ScoreListener->Cancel();
		ScoreListener = nullptr;
	}
}

void UArenaScoreboardStatWidget::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World == ScoreWorld.Get())
	{
		ReleaseScoreListener();
	}
}

void UArenaScoreboardStatWidget::HandleScoreMessage(UAsyncAction_ListenForGameplayMessage* ProxyObject, FGameplayTag Channel)
{
	if (bIsConstructed && ProxyObject == ScoreListener)
	{
		OnScoreChanged();
	}
}
