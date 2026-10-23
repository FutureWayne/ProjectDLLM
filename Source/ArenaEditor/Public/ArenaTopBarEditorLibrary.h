#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaTopBarEditorLibrary.generated.h"
class UWidget;
UCLASS()
class UArenaTopBarEditorLibrary : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Arena|Editor")
 static void DisableDesignerVariable(UWidget* Widget);
};
