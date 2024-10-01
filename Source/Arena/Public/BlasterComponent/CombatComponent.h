// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/HUD/BlasterHUD.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 80000.f

class AArenaPlayerController;
class AWeapon;
class ABlasterCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARENA_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	friend class ABlasterCharacter;

	void EquipWeapon(AWeapon* WeaponToEquip);

	void DropWeapon();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	void SetAiming(bool bNewAiming);

	void Fire();
	void FireButtonPressed(bool bPressed);

	void SetHUDCrosshair(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon() const;

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

private:
	bool bFireButtonPressed;

	float CrosshairVelocityFactor = 0.f;
	float CrosshairInAirFactor = 0.f;
	float CrosshairAimFactor = 0.f;
	float CrosshairShootingFactor = 0.f;

	FVector HitTarget;
	
	TObjectPtr<ABlasterCharacter> OwningCharacter;
	TObjectPtr<AArenaPlayerController> OwningController;
	TObjectPtr<ABlasterHUD> OwningHUD;
	FHUDPackage HUDPackage;

	/*
	 * Aiming and FOV
	 */

	float DefaultFOV;
	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = 0.1, ClampMax = 100))
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AWeapon> EquippedWeapon;

	UPROPERTY(Replicated)
	bool bIsAiming;

	void TraceUnderCrosshair(FHitResult& HitResult);

	void InterpFOV(float DeltaTime);

	/*
	 * Auto fire
	 */
	FTimerHandle FireTimer;

	UPROPERTY(EditAnywhere, Category = "Weapon", meta = (ClampMin = 0.1, ClampMax = 10))
	float FireDelay = .15f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	bool bAutomatic = true;

	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();
};
