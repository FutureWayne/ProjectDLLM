// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaActivatableWidget.h"
#include "ArenaHUDLayout.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable, Meta = (DisplayName = "Arena HUD Layout", Category = "Arena|HUD"))
class ARENA_API UArenaHUDLayout : public UArenaActivatableWidget
{
	GENERATED_BODY()

public:
	UArenaHUDLayout(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

protected:
	void HandleEscapeAction();

protected:
	/**
	 * The menu to be displayed when the user presses the "Pause" or "Escape" button 
	 */
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;
};
