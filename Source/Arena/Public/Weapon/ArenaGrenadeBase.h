// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameplayCueInterface.h"
#include "Equipment/ArenaGrenadeInstance.h"
#include "GameFramework/Actor.h"
#include "ArenaGrenadeBase.generated.h"

class UBoxComponent;
class USphereComponent;
class UArenaGrenadeInstance;
class UGameplayEffect;
class UNiagaraSystem;
class UNiagaraComponent;

USTRUCT()
struct FGrenadeDefinition
{
	GENERATED_BODY()
	
};

class UProjectileMovementComponent;

UCLASS()
class ARENA_API AArenaGrenadeBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArenaGrenadeBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void SetGrenadeParameter(const FGrenadeParams InGrenadeParams);

	UFUNCTION(BlueprintCallable)
	const FGrenadeParams& GetGrenadeParameter() const { return GrenadeParams; }

	UFUNCTION()
	void LaunchGrenade();

protected:
	//~Begin AActor interface
	virtual void BeginPlay() override;
	//~End of AActor interface

	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	virtual void Detonate();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool ShouldDetonate(FHitResult HitResult) const;

private:
	void SetupVFX();
	
	void PostLaunchCleanup();

	bool GetActorsWithinExplosionRadius(TArray<AActor*>& OutOverlappingActors) const;

	void ApplyDamageToTarget(const AActor* Target, const FHitResult& HitResult, bool IsDirectHit) const;

	void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	void SpawnCosmeticActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> TrailComponent;

	UPROPERTY()
	FGrenadeParams GrenadeParams;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionComponent;

private:
	TWeakObjectPtr<AActor> DirectHitTarget;
	
	FLinearColor TeamColor;
	
	FTimerHandle ExplosionCountdownTimerHandle;

	TWeakObjectPtr<AActor> SpawnedCosmeticActor;
	
	bool bDetonationFired = false;
};
