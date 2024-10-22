// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/HUD/BlasterHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/ArenaPlayerController.h"
#include "Weapon/Weapon.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsAiming = false;
	bFireButtonPressed = false;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bIsAiming);
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (OwningCharacter)
	{
		if (OwningCharacter->GetFollowCamera())
		{
			DefaultFOV = OwningCharacter->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OwningCharacter && OwningCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		HitTarget = HitResult.ImpactPoint;

		InterpFOV(DeltaTime);
		SetHUDCrosshair(DeltaTime);
	}
}

void UCombatComponent::SetAiming(const bool bNewAiming)
{
	bIsAiming = bNewAiming;
	ServerSetAiming(bNewAiming);
}

void UCombatComponent::OnRep_EquippedWeapon() const
{
	if (EquippedWeapon && OwningCharacter)
	{
		OwningCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}

void UCombatComponent::TraceUnderCrosshair(FHitResult& HitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrossHairWorldPosition;
	FVector CrossHairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrossHairWorldPosition,
		CrossHairWorldDirection);

	if (bScreenToWorld)
	{
		FVector Start = CrossHairWorldPosition;
		if (OwningCharacter)
		{
			float DistanceToCharacter = (OwningCharacter->GetActorLocation() - Start).Size();
			Start += CrossHairWorldDirection * (DistanceToCharacter + 20.0f);
		}
		
		FVector End = Start + CrossHairWorldDirection * TRACE_LENGTH;

		GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Start,
			End,
			ECC_Visibility);

		if (HitResult.GetActor() && HitResult.GetActor()->Implements<UInteractWithCrosshairInterface>())
		{
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
		
		if (!HitResult.bBlockingHit)
		{
			HitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (bIsAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, ZoomedFOV, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (OwningCharacter && OwningCharacter->GetFollowCamera())
	{
		OwningCharacter->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || OwningCharacter == nullptr)
	{
		bCanFire = true;
		return;
	}

	OwningCharacter->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		FireDelay
		);
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true;
	if (bFireButtonPressed && bAutomatic)
	{
		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}
	
	if (OwningCharacter)
	{
		OwningCharacter->PlayFireMontage();
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::Fire()
{
	if (bCanFire)
	{
		bCanFire = false;
		
		ServerFire(HitTarget);

		if (EquippedWeapon)
		{
			CrosshairShootingFactor = 1.f;
		}
	
		StartFireTimer();
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::SetHUDCrosshair(float DeltaTime)
{
	if (OwningCharacter == nullptr || OwningCharacter->Controller == nullptr)
	{
		return;
	}

	if (OwningController == nullptr)
	{
		OwningController = Cast<AArenaPlayerController>(OwningCharacter->Controller);
	}
	
	if (OwningController)
	{
		if (OwningHUD == nullptr)
		{
			OwningHUD = Cast<ABlasterHUD>(OwningController->GetHUD());
		}
		
		if (OwningHUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
				HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
				HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
				HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
				HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
			}
			else
			{
				HUDPackage.CrosshairCenter = nullptr;
				HUDPackage.CrosshairLeft = nullptr;
				HUDPackage.CrosshairRight = nullptr;
				HUDPackage.CrosshairTop = nullptr;
				HUDPackage.CrosshairBottom = nullptr;
			}

			// Calculate crosshair spread
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetOwner());
			if (BlasterCharacter != nullptr)
			{
				const FVector2D SpeedRange(0.f, BlasterCharacter->GetDefaultJogSpeed());
				const FVector2D VelocityMultiplierRange(0.f, 1.f);
				FVector Velocity = BlasterCharacter->GetVelocity();
				Velocity.Z = 0.f;
				CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(SpeedRange, VelocityMultiplierRange, Velocity.Size());

				if (BlasterCharacter->GetCharacterMovement()->IsFalling())
				{
					CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.f, DeltaTime, 1.f);
				}
				else
				{
					CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 1.f);
				}

				if (bIsAiming)
				{
					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.5f, DeltaTime, 30.f);
				}
				else
				{
					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
				}

				CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 20.f);
				
				HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairShootingFactor - CrosshairAimFactor;
			}
			
			OwningHUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::ServerSetAiming_Implementation(const bool bNewAiming)
{
	bIsAiming = bNewAiming;
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (OwningCharacter == nullptr || WeaponToEquip == nullptr)
	{
		return;
	}

	if (EquippedWeapon)
	{
		DropWeapon();
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	EquippedWeapon->GetWeaponMesh()->SetSimulatePhysics(false);
	EquippedWeapon->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquippedWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EquippedWeapon->ShowPickupWidget(false);
	EquippedWeapon->SetOwner(OwningCharacter);
	
	if (const USkeletalMeshSocket* WeaponSocket = OwningCharacter->GetMesh()->GetSocketByName(FName("weapon_r")))
	{
		WeaponSocket->AttachActor(EquippedWeapon, OwningCharacter->GetMesh());
	}
	
	OwningCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
}

void UCombatComponent::DropWeapon()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	OwningCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
	
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Dropped);
	EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	EquippedWeapon->GetAreaSphere()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	EquippedWeapon->GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	EquippedWeapon->GetWeaponMesh()->SetSimulatePhysics(true);

	// Add a force to the weapon, using the character's aim rotation
	const FVector ForceVector = OwningCharacter->GetBaseAimRotation().Vector() * 400.f;
	EquippedWeapon->GetWeaponMesh()->AddImpulse(ForceVector, NAME_None, true);
	EquippedWeapon->SetOwner(nullptr);
	EquippedWeapon = nullptr;
}

