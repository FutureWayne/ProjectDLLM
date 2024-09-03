// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/ArenaUserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UCharacterOverlay : public UArenaUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<class UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<class UTextBlock> HealthText;

protected:
	UFUNCTION()
	void OnHealthChanged(float NewHealth);
	
	UFUNCTION()
	void OnMaxHealthChanged(float NewMaxHealth);
	
	virtual void OnWidgetControllerSet_Implementation() override;

private:
	float Health = 0.0f;
	float MaxHealth = 0.0f;
};
