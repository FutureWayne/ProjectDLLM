// Copyright Ludens Studio. All Rights Reserved.

#include "UI/ArenaLoadingScreenSubsystem.h"

#include "Blueprint/UserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaLoadingScreenSubsystem)

class UUserWidget;

//////////////////////////////////////////////////////////////////////
// UArenaLoadingScreenSubsystem

UArenaLoadingScreenSubsystem::UArenaLoadingScreenSubsystem()
{
}

void UArenaLoadingScreenSubsystem::SetLoadingScreenContentWidget(TSubclassOf<UUserWidget> NewWidgetClass)
{
	if (LoadingScreenWidgetClass != NewWidgetClass)
	{
		LoadingScreenWidgetClass = NewWidgetClass;

		OnLoadingScreenWidgetChanged.Broadcast(LoadingScreenWidgetClass);
	}
}

TSubclassOf<UUserWidget> UArenaLoadingScreenSubsystem::GetLoadingScreenContentWidget() const
{
	return LoadingScreenWidgetClass;
}



