// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArenaHUD.generated.h"

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
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCharacterOverlay> OverlayWidget;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UAgentChooseWidget> AgentChooseWidget;

	TObjectPtr<UCooldownWidget> CooldownWidget;

	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& InWidgetControllerParams);

	void InitOverlay(APlayerController* PlayerController, APlayerState* PlayerState, UAbilitySystemComponent* AbilitySystemComponent, UAttributeSet* AttributeSet);

	void AddAgentChooseWidget();

	void AddCooldownWidget();
	
protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCharacterOverlay> OverlayWidgetClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAgentChooseWidget> AgentChooseWidgetClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCooldownWidget> CooldownWidgetClass;

	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;
};
