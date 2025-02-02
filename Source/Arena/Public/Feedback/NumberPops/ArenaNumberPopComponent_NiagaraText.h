// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Feedback/NumberPops/ArenaNumberPopComponent.h"
#include "ArenaNumberPopComponent_NiagaraText.generated.h"

class UNiagaraComponent;
class UArenaDamagePopStyleNiagara;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaNumberPopComponent_NiagaraText : public UArenaNumberPopComponent
{
	GENERATED_BODY()

public:
	UArenaNumberPopComponent_NiagaraText(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ~UArenaNumberPopComponent interface
	virtual void AddNumberPop(const FArenaNumberPopRequest& NewRequest) override;
	// ~UArenaNumberPopComponent interface

protected:
	TArray<int32> DamageNumberArray;

	UPROPERTY(EditDefaultsOnly, Category = "Number Pop|Style")
	TObjectPtr<UArenaDamagePopStyleNiagara> Style;

	// Niagara Component used to display the damage
	UPROPERTY(BlueprintReadOnly, Category = "Number Pop|Style")
	TObjectPtr<UNiagaraComponent> NiagaraComp;
};
