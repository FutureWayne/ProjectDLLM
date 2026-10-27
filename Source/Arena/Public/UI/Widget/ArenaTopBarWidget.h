// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArenaTopBarWidget.generated.h"

/** Native TDM HUD. Reads the existing replicated scoring component; never owns match state. */
UCLASS()
class ARENA_API UArenaTopBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UArenaTopBarWidget(const FObjectInitializer& Initializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual void NativeDestruct() override;

private:
	friend class FArenaTopBarSnapshotTest;

	void RefreshSnapshot();

	UPROPERTY(Transient)
	TObjectPtr<UObject> DisplayFont = nullptr;

	TWeakObjectPtr<UObject> ScoringSource;
	int32 Kills[2] = {0, 0};
	int32 TargetKills = 0;
	int32 Seconds = 0;
	float Progress[2] = {0.0f, 0.0f};
	float Pulse[2] = {0.0f, 0.0f};
	float SampleClock = 0.0f;
	float AnimationClock = 0.0f;
	bool bInitialized = false;
	bool bHasData = false;
};
