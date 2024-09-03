// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/CharacterOverlay.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UI/WidgetController/OverlayWidgetController.h"

void UCharacterOverlay::OnHealthChanged(const float NewHealth)
{
	// Health = NewHealth;
	// HealthBar->SetPercent(Health / MaxHealth);
	// HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Health, MaxHealth)));
}

void UCharacterOverlay::OnMaxHealthChanged(const float NewMaxHealth)
{
	// MaxHealth = NewMaxHealth;
	// HealthBar->SetPercent(Health / MaxHealth);
	// HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Health, MaxHealth)));
}

void UCharacterOverlay::OnWidgetControllerSet_Implementation()
{
	// UOverlayWidgetController* OverlayWidgetController = CastChecked<UOverlayWidgetController>(WidgetController);
	// OverlayWidgetController->OnHealthChanged.AddDynamic(this, &UCharacterOverlay::OnHealthChanged);
	// OverlayWidgetController->OnMaxHealthChanged.AddDynamic(this, &UCharacterOverlay::OnMaxHealthChanged);
}
