// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ArenaCharacter.h"
#include "Arena/ArenaTypes/TurnInPlace.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairInterface.h"
#include "BlasterCharacter.generated.h"

class UCombatComponent;
class AWeapon;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class ARENA_API ABlasterCharacter : public AArenaCharacter, public IInteractWithCrosshairInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_ReplicatedMovement() override;

	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	AWeapon* GetEquippedWeapon() const;
	
	void PlayFireMontage();
	void PlayHitReactMontage();
	
	FVector GetHitTarget() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void EquipButtonPressed();

	void DropButtonPressed();

	void CrouchButtonPressed();

	void AimButtonPressed();
	void AimButtonReleased();

	void CalculateAOPitch();
	void AimOffset(float DeltaSeconds);
	void SimProxiesTurn();

	void FireButtonPressed();
	void FireButtonReleased();

	void WalkButtonPressed();
	void WalkButtonReleased();
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float DefaultJogSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float DefaultWalkSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float CameraThreshold = 200.f;
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	// /** MappingContext */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	// UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Equip Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EquipAction;

	/** Drop Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DropAction;

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;

	/** Aim Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;

	/** Walk Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* WalkAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AWeapon> OverlappingWeapon;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCombatComponent> Combat;

	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> HitReactMontage;

	/* Player Health */
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "PlayerStats")
	float Health = 100.f;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UFUNCTION(Server, Reliable)
	void ServerDropButtonPressed();

	UFUNCTION()
	void OnRep_OverlappingWeapon(const AWeapon* LastWeapon) const;

	UFUNCTION()
	void OnRep_Health();

	float AO_Yaw;
	float Interp_AO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	ETurnInPlaceDirection TurnInPlaceDirection;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	float ProxyYawDelta = 0.0f;
	float TimeSinceLastMovementReplication = 0.0f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;

	void TurnInPlace(float DeltaSeconds);

	void HideCharacterIfCameraClose();

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE float GetAOYaw() const { return AO_Yaw; }
	FORCEINLINE float GetAOPitch() const { return AO_Pitch; }
	FORCEINLINE float GetDefaultJogSpeed() const { return DefaultJogSpeed; }
	FORCEINLINE ETurnInPlaceDirection GetTurnInPlaceDirection() const { return TurnInPlaceDirection; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
};
