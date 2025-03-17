// Copyright Ludens Studio. All Rights Reserved.

#include "UI/ArenaTabButtonBase.h"

#include "CommonLazyImage.h"
#include "UI/ArenaTabListWidgetBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaTabButtonBase)

class UObject;
struct FSlateBrush;

void UArenaTabButtonBase::SetIconFromLazyObject(TSoftObjectPtr<UObject> LazyObject)
{
	if (LazyImage_Icon)
	{
		LazyImage_Icon->SetBrushFromLazyDisplayAsset(LazyObject);
	}
}

void UArenaTabButtonBase::SetIconBrush(const FSlateBrush& Brush)
{
	if (LazyImage_Icon)
	{
		LazyImage_Icon->SetBrush(Brush);
	}
}

void UArenaTabButtonBase::SetTabLabelInfo_Implementation(const FArenaTabDescriptor& TabLabelInfo)
{
	SetButtonText(TabLabelInfo.TabText);
	SetIconBrush(TabLabelInfo.IconBrush);
}



