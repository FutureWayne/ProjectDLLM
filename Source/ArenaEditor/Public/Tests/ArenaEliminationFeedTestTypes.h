// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameFramework/GameMode.h"
#include "UI/Widget/ArenaEliminationFeedWidget.h"
#include "ArenaEliminationFeedTestTypes.generated.h"

/** Editor-only access to widget lifecycle and configuration boundaries. */
UCLASS(Transient)
class UArenaEliminationFeedTestWidget : public UArenaEliminationFeedWidget
{
	GENERATED_BODY()

public:
	void ConstructForTest();
	void DestructForTest() { NativeDestruct(); }
	void ConfigureForTest(float Lifetime, int32 Limit);
	UListView* GetListForTest() const { return ToastListWidget; }
};

/** Minimal real PIE server/client fixture using the production GameState RPCs. */
UCLASS(Transient)
class AArenaEliminationFeedTestGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AArenaEliminationFeedTestGameMode();
};
