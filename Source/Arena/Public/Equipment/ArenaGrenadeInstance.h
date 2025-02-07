// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Equipment/ArenaEquipmentInstance.h"
#include "ArenaGrenadeInstance.generated.h"

class AArenaGrenadeBase;
class UGameplayEffect;
class UNiagaraSystem;

USTRUCT(Blueprintable)
struct FGrenadeParams
{
	GENERATED_BODY()

	FGrenadeParams() {}

public:
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
};

/**
 * 
 */
UCLASS()
class ARENA_API UArenaGrenadeInstance : public UArenaEquipmentInstance
{
	GENERATED_BODY()

public:
	UArenaGrenadeInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	UPROPERTY(EditDefaultsOnly)
	FGrenadeParams GrenadeParams;

public:
	UFUNCTION(BlueprintCallable)
	const FGrenadeParams& GetGrenadeParams() const;
};
