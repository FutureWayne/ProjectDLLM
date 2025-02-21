// Copyright Ludens Studio. All Rights Reserved.


#include "UI/ArenaActivatableWidget.h"

UArenaActivatableWidget::UArenaActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TOptional<FUIInputConfig> UArenaActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputConfig)
	{
		case EArenaWidgetInputMode::GameAndMenu:
			return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode);
		case EArenaWidgetInputMode::Game:
			return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);
		case EArenaWidgetInputMode::Menu:
			return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
		case EArenaWidgetInputMode::Default:
		default:
			return TOptional<FUIInputConfig>();
	}
}
