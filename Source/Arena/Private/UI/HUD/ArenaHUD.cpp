// Copyright Ludens Studio. All Rights Reserved.


#include "UI/HUD/ArenaHUD.h"
#include "ArenaGameplayTags.h"

#include "CommonUIExtensions.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_GAME, "UI.Layer.Game");

void AArenaHUD::BeginPlay()
{
	Super::BeginPlay();

	ULocalPlayer* LocalPlayer = nullptr;
	if (APlayerController*  PlayerController = GetOwningPlayerController())
	{
		LocalPlayer = PlayerController->GetLocalPlayer();
	}

	if (LocalPlayer)
	{
		OverlayWidget = UCommonUIExtensions::PushContentToLayer_ForPlayer(LocalPlayer, TAG_UI_LAYER_GAME, OverlayWidgetClass);
	}
}

void AArenaHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(GetOwningPlayerController()->Player))
	{
		UCommonUIExtensions::PopContentFromLayer(OverlayWidget);
	}
}
