// Copyright Ludens Studio. All Rights Reserved.


#include "UI/HUD/ArenaHUD.h"
#include "ArenaGameplayTags.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/ArenaReticleWidget.h"

#include "CommonUIExtensions.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_GAME, "UI.Layer.Game");

void AArenaHUD::ShowEnemyHit(EArenaHitFeedbackType FeedbackType)
{
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UArenaReticleWidget::StaticClass(), false);
	for (UUserWidget* Widget : Widgets)
	{
		if (Widget->GetOwningPlayer() == GetOwningPlayerController() && Widget->IsVisible())
		{
			CastChecked<UArenaReticleWidget>(Widget)->ShowEnemyHit(FeedbackType);
		}
	}
}

void AArenaHUD::AddOverlayWidget()
{
	ULocalPlayer* LocalPlayer = nullptr;
	if (APlayerController*  PlayerController = GetOwningPlayerController())
	{
		LocalPlayer = PlayerController->GetLocalPlayer();
	}
	
	if (LocalPlayer)
	{
		RemoveOverlayWidget();
		OverlayWidget = UCommonUIExtensions::PushContentToLayer_ForPlayer(LocalPlayer, TAG_UI_LAYER_GAME, OverlayWidgetClass);
	}
}

void AArenaHUD::RemoveOverlayWidget()
{
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(GetOwningPlayerController()->Player);
	if (LocalPlayer && OverlayWidget)
	{
		UCommonUIExtensions::PopContentFromLayer(OverlayWidget);
	}
}

void AArenaHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AArenaHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	RemoveOverlayWidget();
}
