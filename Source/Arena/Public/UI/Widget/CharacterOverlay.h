// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/ArenaUserWidget.h"
#include "CharacterOverlay.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class ARENA_API UCharacterOverlay : public UArenaUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MatchCountdownText;
};
