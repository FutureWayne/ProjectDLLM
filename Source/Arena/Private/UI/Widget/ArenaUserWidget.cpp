// Copyright Ludens Studio. All Rights Reserved.


#include "UI/Widget/ArenaUserWidget.h"

void UArenaUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	OnWidgetControllerSet();
}

void UArenaUserWidget::OnWidgetControllerSet_Implementation()
{
}
