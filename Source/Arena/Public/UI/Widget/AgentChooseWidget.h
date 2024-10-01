// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ArenaUserWidget.h"
#include "AgentChooseWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class ARENA_API UAgentChooseWidget : public UArenaUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ChooseTimeCountdown;
};
