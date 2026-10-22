// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "UI/Widget/ArenaHealthBarWidget.h"
#include "ArenaHealthBarTestWidget.generated.h"

/** Records native presentation events for healthbar lifecycle regression tests. */
UCLASS()
class UArenaHealthBarTestWidget : public UArenaHealthBarWidget
{
	GENERATED_BODY()

public:
	void ConstructForTest() { NativeConstruct(); }
	void DestructForTest() { NativeDestruct(); }

	int32 SnapshotCount = 0;
	int32 ChangeCount = 0;
	float LastHealth = 0.0f;
	float LastNormalizedHealth = 0.0f;
	float LastOldNormalizedHealth = 0.0f;
	bool bLastHealing = false;
	bool bLastOutOfHealth = false;

protected:
	virtual void InitializeHealthbarVisuals_Implementation(float Health, float NormalizedHealth) override;
	virtual void PresentHealthbarChange_Implementation(float OldHealth, float NewHealth, float OldNormalizedHealth, float NewNormalizedHealth, bool bHealing, bool bOutOfHealth) override;
};
