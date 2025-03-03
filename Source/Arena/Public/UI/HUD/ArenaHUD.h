// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

class UCommonActivatableWidget;
class UCooldownWidget;
class UCharacterOverlay;
class UAgentChooseWidget;
struct FWidgetControllerParams;
class UOverlayWidgetController;
class UArenaUserWidget;
class UAbilitySystemComponent;
class UAttributeSet;

/**
 * 
 */
UCLASS()
class ARENA_API AArenaHUD : public AHUD
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<UCommonActivatableWidget> OverlayWidgetClass;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	TObjectPtr<UCommonActivatableWidget> OverlayWidget;
};
