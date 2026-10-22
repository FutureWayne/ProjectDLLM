// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "ArenaHealthBarWidget.generated.h"

class APawn;
class APlayerController;
class UArenaHealthComponent;

/** Owns health observation and presentation events; Blueprint owns the bar's appearance. */
UCLASS(Abstract, Blueprintable)
class ARENA_API UArenaHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Create dynamic materials before receiving the initial health snapshot. */
	UFUNCTION(BlueprintNativeEvent, Category = "Arena|Health|Presentation")
	void PrepareHealthbarVisuals();

	/** Reset animations and display the snapshot without playing a damage/healing transition. */
	UFUNCTION(BlueprintNativeEvent, Category = "Arena|Health|Presentation")
	void InitializeHealthbarVisuals(float Health, float NormalizedHealth);

	/** All values are computed in C++; Blueprint only applies materials, text and animations. */
	UFUNCTION(BlueprintNativeEvent, Category = "Arena|Health|Presentation")
	void PresentHealthbarChange(float OldHealth, float NewHealth, float OldNormalizedHealth, float NewNormalizedHealth, bool bHealing, bool bOutOfHealth);

private:
	void SetHealthSource(APawn* Pawn);
	void UnbindHealthSource();
	void RefreshHealthSnapshot();

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleHealthChanged(UArenaHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	UFUNCTION()
	void HandleMaxHealthChanged(UArenaHealthComponent* HealthComponent, float OldValue, float NewValue, AActor* Instigator);

	TWeakObjectPtr<APlayerController> ObservedController;
	TWeakObjectPtr<UArenaHealthComponent> ObservedHealthComponent;
};
