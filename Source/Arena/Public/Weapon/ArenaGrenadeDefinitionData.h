// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ArenaGameplayTags.h"
#include "ArenaGrenadeDefinitionData.generated.h"

class UArenaGrenadeDefinitionData;
class AArenaEffectActor;
class UNiagaraSystem;
class AArenaGrenadeBase;
class UGameplayEffect;

ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_DetonationPolicy_OnImpact);
ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_DetonationPolicy_OnHitValidTarget);
ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_DetonationPolicy_OnHitHorizontal);

UENUM(BlueprintType)
enum class ESpawnPolicy : uint8
{
	SpawnOnHit,
	SpawnOnDetonation,
	DoNotSpawn
};

USTRUCT(BlueprintType)
struct FEffectActorSpawnData
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EffectActor")
	ESpawnPolicy SpawnPolicy = ESpawnPolicy::DoNotSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EffectActor")
	TSubclassOf<AArenaEffectActor> EffectActorClass = nullptr;
};

USTRUCT(BlueprintType)
struct FSecondaryGrenadeSpawnData
{	
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SecondaryGrenade")
	ESpawnPolicy SpawnPolicy = ESpawnPolicy::DoNotSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SecondaryGrenade")
	TObjectPtr<UArenaGrenadeDefinitionData> GrenadeDefinitionData = nullptr;
};

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Projectile")
	float Bounciness = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Projectile")
	float Friction = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Projectile")
	float MinFrictionFraction = 0.8f;
	
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Behavior")
	float TimeBeforeExplosion = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Behavior")
	float DetonationRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Behavior")
	TSubclassOf<UGameplayEffect> ExplosionGameplayEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Behavior")
	TSubclassOf<UGameplayEffect> DirectHitGameplayEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Behavior")
	FGameplayTagContainer DetonationPolicy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Effect")
	TArray<FEffectActorSpawnData> EffectActorsToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|Effect")
	TArray<FSecondaryGrenadeSpawnData> SecondaryGrenadesToSpawn;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade|GrenadeClass", meta = (ToolTip = "Use B_GrenadeBase if no special function required"))
	TSubclassOf<AArenaGrenadeBase> GrenadeClass = nullptr;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
