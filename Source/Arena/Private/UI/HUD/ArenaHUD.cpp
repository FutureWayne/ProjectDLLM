// Copyright Ludens Studio. All Rights Reserved.


#include "UI/HUD/ArenaHUD.h"

#include "UI/Widget/ArenaUserWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"

UOverlayWidgetController* AArenaHUD::GetOverlayWidgetController(const FWidgetControllerParams& InWidgetControllerParams)
{
	if (OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(InWidgetControllerParams);
	}

	return OverlayWidgetController;
}

void AArenaHUD::InitOverlay(APlayerController* PlayerController, APlayerState* PlayerState,
	UAbilitySystemComponent* AbilitySystemComponent, UAttributeSet* AttributeSet)
{
	checkf(OverlayWidgetClass, TEXT("OverlayWidgetClass is not set in %s"), *GetName());
	checkf(OverlayWidgetControllerClass, TEXT("OverlayWidgetControllerClass is not set in %s"), *GetName());
	
	const FWidgetControllerParams WidgetControllerParams(PlayerController, PlayerState, AbilitySystemComponent, AttributeSet);
	UOverlayWidgetController* WidgetController = GetOverlayWidgetController(WidgetControllerParams);
	
	OverlayWidget = CreateWidget<UArenaUserWidget>(GetWorld(), OverlayWidgetClass);
	OverlayWidget->SetWidgetController(WidgetController);
	WidgetController->BroadCastInitialValues();
	WidgetController->BindCallbacksToDependencies();
	OverlayWidget->AddToViewport();
}

void AArenaHUD::BeginPlay()
{
	Super::BeginPlay();
}
