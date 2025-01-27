// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ArenaCharacter.h"
#include "Arena/ArenaTypes/TurnInPlace.h"
#include "GameFramework/Character.h"
#include "Interfaces/InteractWithCrosshairInterface.h"
#include "BlasterCharacter.generated.h"

class UWidgetComponent;
enum class ETeam : uint8;
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

	UFUNCTION(BlueprintCallable)
	bool IsWeaponEquipped() const;
	
	bool IsAiming() const;
	AWeapon* GetEquippedWeapon() const;
	
	void PlayFireMontage();
	void PlayHitReactMontage();
	
	FVector GetHitTarget() const;

	void SetTeamColor(int32 TeamId) const;
	void SetSpawnPoint();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	USkeletalMeshComponent* GetDisplayMesh();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Input functions
	 */
	void Move(const FInputActionValue& Value);
	
	void Look(const FInputActionValue& Value);

	void EquipButtonPressed();

	void DropButtonPressed();

	void CrouchButtonPressed();

	void AimButtonPressed();
	void AimButtonReleased();

	void FireButtonPressed();
	void FireButtonReleased();

	void WalkButtonPressed();
	void WalkButtonReleased();

	void CalculateAOPitch();
	void AimOffset(float DeltaSeconds);
	void SimProxiesTurn();

	virtual void OnPlayerStateInitialized() override;
	
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

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidget;

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

	/*
	 * Team Colors
	 */

	UPROPERTY(EditAnywhere, Category = "TeamColors")
	TObjectPtr<UMaterialInstance> RedMaterial1;

	UPROPERTY(EditAnywhere, Category = "TeamColors")
	TObjectPtr<UMaterialInstance> RedMaterial2;

	UPROPERTY(EditAnywhere, Category = "TeamColors")
	TObjectPtr<UMaterialInstance> BlueMaterial1;

	UPROPERTY(EditAnywhere, Category = "TeamColors")
	TObjectPtr<UMaterialInstance> BlueMaterial2;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UFUNCTION(Server, BlueprintCallable, Reliable)
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

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArenaEquipmentManagerComponent> EquipmentManagerComponent;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	FORCEINLINE float GetAOYaw() const { return AO_Yaw; }
	FORCEINLINE float GetAOPitch() const { return AO_Pitch; }
	FORCEINLINE float GetDefaultJogSpeed() const { return DefaultJogSpeed; }
	FORCEINLINE ETurnInPlaceDirection GetTurnInPlaceDirection() const { return TurnInPlaceDirection; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
};
