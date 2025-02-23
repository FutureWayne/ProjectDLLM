// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "ArenaGrenadeBase.generated.h"

class AArenaEffectActor;
class UArenaGrenadeDefinitionData;
class UBoxComponent;
class USphereComponent;
class UGameplayEffect;
class UNiagaraSystem;
class UNiagaraComponent;

class UProjectileMovementComponent;

UCLASS()
class ARENA_API AArenaGrenadeBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArenaGrenadeBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void SetGrenadeParameter(const UArenaGrenadeDefinitionData* InGrenadeDefinitionData);

	UFUNCTION(BlueprintCallable)
	const UArenaGrenadeDefinitionData* GetGrenadeDefinitionData() { return GrenadeDefinitionData; }

protected:
	//~Begin AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	virtual void Detonate();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grenade Override")
	void PostDetonation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grenade Override")
	bool ShouldDetonateOnImpact(FHitResult HitResult) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grenade Override")
	void SpawnEffectActor(const FTransform& SpawnTransform, TSubclassOf<AArenaEffectActor> EffectActorClass);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grenade Override")
	void SpawnSecondaryGrenade(const FTransform& SpawnTransform, const UArenaGrenadeDefinitionData* GrenadeDefinition);

private:
	void LaunchGrenade();
	
	void SetupVFX();
	
	void PostLaunchCleanup();

	bool GetActorsWithinExplosionRadius(TArray<AActor*>& OutOverlappingActors) const;

	void ApplyDamageToTarget(const AActor* Target, const FHitResult& HitResult, bool IsDirectHit) const;

	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	void SpawnCosmeticActor();

	void CheckSpawnConditionOnHit(const FHitResult& Hit);

	void CheckSpawnConditionOnDetonation();

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
	
	TWeakObjectPtr<AActor> DirectHitTarget;
	

	
	FTimerHandle ExplosionCountdownTimerHandle;
	
	bool bDetonationFired = false;
};
