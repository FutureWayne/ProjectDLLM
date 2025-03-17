// Copyright Ludens Studio. All Rights Reserved.


#include "UI/ArenaHUDLayout.h"

#include "CommonUIExtensions.h"
#include "NativeGameplayTags.h"
#include "UITag.h"
#include "Input/CommonUIInputTypes.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_ESCAPE, "UI.Action.Escape");

UArenaHUDLayout::UArenaHUDLayout(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UArenaHUDLayout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(TAG_UI_ACTION_ESCAPE), false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleEscapeAction)));
}

void UArenaHUDLayout::NativeDestruct()
{
	Super::NativeDestruct();
}

void UArenaHUDLayout::HandleEscapeAction()
{
	if (ensure(!EscapeMenuClass.IsNull()))
	{
		UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(GetOwningLocalPlayer(), TAG_UI_LAYER_MENU, EscapeMenuClass);
	}
}
