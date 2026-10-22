// Copyright Ludens Studio. All Rights Reserved.

#include "UI/Widget/ArenaHealthBarWidget.h"

#include "Character/ArenaHealthComponent.h"
#include "GameFramework/PlayerController.h"

namespace
{
	float NormalizeHealth(float Health, float MaxHealth)
	{
		return MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;
	}
}

void UArenaHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PrepareHealthbarVisuals();
	ObservedController = GetOwningPlayer();
	if (APlayerController* Controller = ObservedController.Get())
	{
		Controller->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePossessedPawnChanged);
		SetHealthSource(Controller->GetPawn());
	}
	else
	{
		SetHealthSource(nullptr);
	}
}

void UArenaHealthBarWidget::NativeDestruct()
{
	if (APlayerController* Controller = ObservedController.Get())
	{
		Controller->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
	}
	ObservedController.Reset();
	UnbindHealthSource();

	Super::NativeDestruct();
}

void UArenaHealthBarWidget::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	SetHealthSource(NewPawn);
}

void UArenaHealthBarWidget::SetHealthSource(APawn* Pawn)
{
	UnbindHealthSource();
	ObservedHealthComponent = UArenaHealthComponent::FindHealthComponent(Pawn);
	if (UArenaHealthComponent* HealthComponent = ObservedHealthComponent.Get())
	{
		HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleHealthChanged);
		HealthComponent->OnMaxHealthChanged.AddUniqueDynamic(this, &ThisClass::HandleMaxHealthChanged);
	}
	RefreshHealthSnapshot();
}

void UArenaHealthBarWidget::UnbindHealthSource()
{
	if (UArenaHealthComponent* HealthComponent = ObservedHealthComponent.Get())
	{
		HealthComponent->OnHealthChanged.RemoveDynamic(this, &ThisClass::HandleHealthChanged);
		HealthComponent->OnMaxHealthChanged.RemoveDynamic(this, &ThisClass::HandleMaxHealthChanged);
	}
	ObservedHealthComponent.Reset();
}

void UArenaHealthBarWidget::RefreshHealthSnapshot()
{
	const UArenaHealthComponent* HealthComponent = ObservedHealthComponent.Get();
	const float Health = HealthComponent ? HealthComponent->GetHealth() : 0.0f;
	const float MaxHealth = HealthComponent ? HealthComponent->GetMaxHealth() : 0.0f;
	InitializeHealthbarVisuals(Health, NormalizeHealth(Health, MaxHealth));
}

void UArenaHealthBarWidget::HandleHealthChanged(UArenaHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	if (HealthComponent != ObservedHealthComponent.Get())
	{
		return;
	}

	// Initialization broadcasts equal values; refresh without playing a change animation.
	if (OldValue == NewValue)
	{
		RefreshHealthSnapshot();
		return;
	}

	const float MaxHealth = HealthComponent->GetMaxHealth();
	PresentHealthbarChange(OldValue, NewValue, NormalizeHealth(OldValue, MaxHealth), NormalizeHealth(NewValue, MaxHealth), NewValue > OldValue, NewValue <= 0.0f);
}

void UArenaHealthBarWidget::HandleMaxHealthChanged(UArenaHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator)
{
	if (HealthComponent == ObservedHealthComponent.Get())
	{
		RefreshHealthSnapshot();
	}
}

void UArenaHealthBarWidget::PrepareHealthbarVisuals_Implementation()
{
}

void UArenaHealthBarWidget::InitializeHealthbarVisuals_Implementation(float Health, float NormalizedHealth)
{
}

void UArenaHealthBarWidget::PresentHealthbarChange_Implementation(float OldHealth, float NewHealth, float OldNormalizedHealth, float NewNormalizedHealth, bool bHealing, bool bOutOfHealth)
{
}
