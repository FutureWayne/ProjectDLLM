// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

class ACasing;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class ARENA_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ShowPickupWidget(bool bShow) const;

	virtual void Fire(const FVector& HitTarget);

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairCenter;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairLeft;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairRight;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairTop;

	UPROPERTY(EditAnywhere, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairBottom;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	TSubclassOf<ACasing> CasingClass;

	/*
	 * Zoomed FOV
	 */

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = 0.1, ClampMax = 100))
	float ZoomInterpSpeed = 20.f;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<class USphereComponent> AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<class UWidgetComponent> PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	TObjectPtr<UAnimationAsset> FireAnimation;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState() const;

public:
	FORCEINLINE void SetWeaponState(const EWeaponState State) { WeaponState = State; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
};
