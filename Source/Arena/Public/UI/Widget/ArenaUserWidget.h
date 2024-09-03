// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArenaUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API UArenaUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void SetWidgetController(UObject* InWidgetController);

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UObject> WidgetController;

protected:
	UFUNCTION(BlueprintNativeEvent)
	void OnWidgetControllerSet();
};
