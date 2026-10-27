#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaTopBarEditorLibrary.generated.h"
class UWidget;
class UWidgetAnimation;
class UWidgetBlueprint;
UCLASS()
class UArenaTopBarEditorLibrary : public UBlueprintFunctionLibrary
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Arena|Editor")
 static void DisableDesignerVariable(UWidget* Widget);

 UFUNCTION(BlueprintCallable, Category="Arena|Editor")
 static void SetDesignerVariable(UWidget* Widget, bool bIsVariable);

 /** Designer-authoring helper: sets the root of a Widget Blueprint's tree (not exposed to Python by UMG). */
 UFUNCTION(BlueprintCallable, Category="Arena|Editor")
 static bool SetWidgetBlueprintRoot(UWidgetBlueprint* Blueprint, UWidget* Root);

 /** Designer-authoring helper: finds or creates a widget animation, mirroring the UMG designer "+ Animation" action. */
 UFUNCTION(BlueprintCallable, Category="Arena|Editor")
 static UWidgetAnimation* FindOrAddWidgetAnimation(UWidgetBlueprint* Blueprint, FName Name, float DurationSeconds);

 /** Designer-authoring helper: binds a widget of the Blueprint's tree to an animation and returns the binding guid. */
 UFUNCTION(BlueprintCallable, Category="Arena|Editor")
 static FGuid BindWidgetToAnimation(UWidgetAnimation* Animation, UWidget* Widget);
};
