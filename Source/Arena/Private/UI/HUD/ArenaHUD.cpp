// Copyright Ludens Studio. All Rights Reserved.


#include "UI/HUD/ArenaHUD.h"

#include "UI/Widget/AgentChooseWidget.h"
#include "UI/Widget/CharacterOverlay.h"
#include "UI/Widget/CooldownWidget.h"
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

	// If we already have a valid OverlayWidget, remove it before creating a new one
	if (OverlayWidget)
	{
		OverlayWidget->RemoveFromParent();
		OverlayWidget = nullptr;
	}
	
	auto Player = PlayerController->GetPawn();
	OverlayWidget = CreateWidget<UCharacterOverlay>(PlayerController, OverlayWidgetClass);
	OverlayWidget->SetWidgetController(WidgetController);
	WidgetController->BroadCastInitialValues();
	WidgetController->BindCallbacksToDependencies();
	OverlayWidget->AddToViewport();
}

void AArenaHUD::RemoveOverlay()
{
	if (OverlayWidget)
	{
		OverlayWidget->RemoveFromParent();
		OverlayWidget = nullptr;
	}
}

void AArenaHUD::AddAgentChooseWidget()
{
	checkf(AgentChooseWidgetClass, TEXT("AgentChooseWidgetClass is not set in %s"), *GetName());
	AgentChooseWidget = CreateWidget<UAgentChooseWidget>(GetWorld(), AgentChooseWidgetClass);
	AgentChooseWidget->AddToViewport();
}

void AArenaHUD::AddCooldownWidget()
{
	checkf(CooldownWidgetClass, TEXT("CooldownWidgetClass is not set in %s"), *GetName());
	CooldownWidget = CreateWidget<UCooldownWidget>(GetWorld(), CooldownWidgetClass);
	CooldownWidget->AddToViewport();
}

void AArenaHUD::BeginPlay()
{
	Super::BeginPlay();
}
