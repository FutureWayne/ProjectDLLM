// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaDamagePopStyleNiagara.generated.h"

class UNiagaraSystem;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaDamagePopStyleNiagara : public UDataAsset
{
	GENERATED_BODY()

public:
	// The name of the Niagara array to set the DamageInfo
	UPROPERTY(EditDefaultsOnly, Category="DamagePop")
	FName NiagaraArrayName;

	UPROPERTY(EditDefaultsOnly, Category="DamagePop")
	TObjectPtr<UNiagaraSystem> TextNiagara;
};
