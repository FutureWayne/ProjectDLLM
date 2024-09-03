// Copyright Ludens Studio. All Rights Reserved.


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AbilitySystem/ArenaHealthSet.h"

void UOverlayWidgetController::BroadCastInitialValues()
{
	const UArenaHealthSet* HealthSet = CastChecked<UArenaHealthSet>(AttributeSet);

	OnHealthChanged.Broadcast(HealthSet->GetHealth());
	OnMaxHealthChanged.Broadcast(HealthSet->GetMaxHealth());
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{
	const UArenaHealthSet* HealthSet = CastChecked<UArenaHealthSet>(AttributeSet);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(HealthSet->GetHealthAttribute()).AddUObject(this, &UOverlayWidgetController::HealthChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(HealthSet->GetMaxHealthAttribute()).AddUObject(this, &UOverlayWidgetController::MaxHealthChangedCallback);
}

void UOverlayWidgetController::HealthChangedCallback(const FOnAttributeChangeData& Data) const
{
	OnHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::MaxHealthChangedCallback(const FOnAttributeChangeData& Data) const
{
	OnMaxHealthChanged.Broadcast(Data.NewValue);
}
