// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/ArenaUserWidget.h"
#include "CooldownWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class ARENA_API UCooldownWidget : public UArenaUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CooldownTimeCountdown;
};
