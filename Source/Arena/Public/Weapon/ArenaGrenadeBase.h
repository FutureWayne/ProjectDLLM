// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "ArenaGrenadeBase.generated.h"

class AArenaEffectActor;
class UArenaEffectActorDefinition;
class UArenaGrenadeDefinitionData;
class UAbilitySystemComponent;
class UBoxComponent;
class USphereComponent;
class UGameplayEffect;
class UNiagaraSystem;
class UNiagaraComponent;

class UProjectileMovementComponent;

enum class ESpawnPolicy : uint8;

UCLASS()
class ARENA_API AArenaGrenadeBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AArenaGrenadeBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void SetGrenadeParameter(const UArenaGrenadeDefinitionData* InGrenadeDefinitionData);

	/** Replace the remaining fuse; zero or negative time means it has already expired. */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void SetGrenadeExplosionTimer(float TimeBeforeExplosion);

	UFUNCTION(BlueprintCallable)
	const UArenaGrenadeDefinitionData* GetGrenadeDefinitionData() { return GrenadeDefinitionData; }

	/**
	 * Forwards the thrower's ability system to grenades that are spawned from this one.
	 * A chain of submunitions must not lose attribution when the original Pawn is collected.
	 */
	void SetSourceAbilitySystem(UAbilitySystemComponent* InSourceAbilitySystem);

	UAbilitySystemComponent* GetSourceAbilitySystem() const;

	/**
	 * Server-side attribution: the grenade the player threw. Submunitions inherit it so eliminations credit
	 * the thrown grenade rather than an internal submunition definition.
	 */
	void SetSourceGrenadeDefinition(const UArenaGrenadeDefinitionData* InSourceGrenadeDefinition);

	/** The inherited thrown grenade, falling back to this grenade's own definition. */
	const UArenaGrenadeDefinitionData* GetSourceGrenadeDefinition() const;

protected:
	//~Begin AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

	UFUNCTION(BlueprintCallable)
	void SetupVFX();

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	virtual void Detonate();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grenade Override")
	void PostDetonation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grenade Override")
	bool ShouldDetonateOnImpact(FHitResult HitResult) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grenade Override")
	void SpawnEffectActor(const FTransform& SpawnTransform,
		TSubclassOf<AArenaEffectActor> EffectActorClass, const UArenaEffectActorDefinition* EffectDefinition = nullptr,
		int32 SpawnCount = 1, float SpawnRadius = 0.f);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grenade Override")
	void SpawnSecondaryGrenade(const FTransform& SpawnTransform, const UArenaGrenadeDefinitionData* GrenadeDefinition, int32 SpawnCount);

private:
	/** Dispatch EffectActorsToSpawn entries matching Policy to SpawnEffectActor. */
	void ProcessEffectActorEntries(const FTransform& SpawnTransform, ESpawnPolicy Policy);

	/** Dispatch SecondaryGrenadesToSpawn entries matching Policy to SpawnSecondaryGrenade. */
	void ProcessSecondaryGrenadeEntries(const FTransform& SpawnTransform, ESpawnPolicy Policy);

	void LaunchGrenade();

	void PrepareExplosionCue();

	void PostLaunchCleanup();

	bool GetActorsWithinExplosionRadius(TArray<AActor*>& OutOverlappingActors) const;

	void ApplyDamageToTarget(const AActor* Target, const FHitResult& HitResult, bool bIsDirectHit) const;

	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	void SpawnCosmeticActor();

	void SpawnActorsOnHit(const FHitResult& Hit);

	void SpawnActorsOnDetonation();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> TrailComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> SpawnedCosmeticActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	FLinearColor TeamColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug", meta = (AllowPrivateAccess = "true"))
	bool bDrawDebug = false;

private:
	UPROPERTY(Replicated)
	TObjectPtr<const UArenaGrenadeDefinitionData> GrenadeDefinitionData;

	/**
	 * Captured at launch, before the thrower can die.
	 * For players this is the PlayerState-owned component, so damage attribution survives the Pawn.
	 */
	TWeakObjectPtr<UAbilitySystemComponent> SourceAbilitySystem;

	UPROPERTY(Transient)
	TObjectPtr<const UArenaGrenadeDefinitionData> SourceGrenadeDefinition;

	TWeakObjectPtr<AActor> DirectHitTarget;

	FTimerHandle ExplosionCountdownTimerHandle;

	bool bDetonationFired = false;
};
