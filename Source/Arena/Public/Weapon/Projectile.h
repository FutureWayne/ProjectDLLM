// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UGameplayEffect;
class UProjectileMovementComponent;
class USoundCue;

UCLASS()
class ARENA_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();

	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UBoxComponent> CollisionBox;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> Tracer;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UParticleSystem> ImpactEffect;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundCue> ImpactSound;
	
	TObjectPtr<UParticleSystemComponent> TracerComponent;

};
