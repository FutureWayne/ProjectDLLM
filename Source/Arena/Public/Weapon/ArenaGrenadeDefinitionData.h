// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ArenaGrenadeDefinitionData.generated.h"

class UNiagaraSystem;
class AArenaGrenadeBase;
class UGameplayEffect;
/**
 * 
 */
UCLASS()
class ARENA_API UArenaGrenadeDefinitionData : public UDataAsset
{
	GENERATED_BODY()

public:
	UArenaGrenadeDefinitionData(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Projectile")
	float ProjectileSpeed = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Projectile")
	bool bShouldBounce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Projectile")
	float GravityScale = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cosmetic")
	TObjectPtr<UNiagaraSystem> TrailEffect = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cosmetic")
	TObjectPtr<USoundBase> GrenadeImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cosmetic")
	FGameplayTag ExplosionCueTag = FGameplayTag::EmptyTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cosmetic")
	TObjectPtr<UAnimMontage> ThrowMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Cosmetic")
	TSubclassOf<AActor> CosmeticActorClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Damage")
	float TimeBeforeExplosion = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Damage")
	float DetonationRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Damage")
	TSubclassOf<UGameplayEffect> ExplosionGameplayEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Damage")
	TSubclassOf<UGameplayEffect> DirectHitGameplayEffect = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|GrenadeClass", meta = (TooTip = "Use B_GrenadeBase if no special function required"))
	TSubclassOf<AArenaGrenadeBase> GrenadeClass = nullptr;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
