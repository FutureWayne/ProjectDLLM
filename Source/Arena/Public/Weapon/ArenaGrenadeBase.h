// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "GameplayCueInterface.h"
#include "GameFramework/Actor.h"
#include "ArenaGrenadeBase.generated.h"

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
	AArenaGrenadeBase();

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

	void ApplyDamageToTarget(const AActor* Target, const FHitResult& HitResult) const;

	void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
	
	UFUNCTION()
	void OnTrailFinished(UNiagaraComponent* PSystem);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> TrailComponent;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float TimeBeforeExplosion = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float DetonationRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	float ProjectileSpeed = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	bool bShouldBounce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<UNiagaraSystem> TrailEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	FGameplayTag ExplosionCueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TSubclassOf<UGameplayEffect> ExplosionGameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TSubclassOf<UGameplayEffect> DirectHitGameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<USoundBase> GrenadeImpactSound;

private:
	TWeakObjectPtr<AActor> DirectHitTarget;
	
	FLinearColor TeamColor;
	
	FTimerHandle ExplosionCountdownTimerHandle;
	
	bool bDetonationFired = false;
};
