// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaGroundAreaVfxEditorLibrary.generated.h"

class UNiagaraSystem;

/** Authoring helper for the dedicated falling fire asset; runtime values remain editable in Niagara. */
UCLASS()
class UArenaGroundAreaVfxEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Ground Area|Authoring")
	static UNiagaraSystem* CreateFallingFireSystem(float FlameDensity = 10.f);
};
