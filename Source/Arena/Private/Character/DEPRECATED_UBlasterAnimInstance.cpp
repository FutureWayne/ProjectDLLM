// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DEPRECATED_UBlasterAnimInstance.h"

#include "Character/DEPRECATED_ABlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

void UDEPRECATED_UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter_DEPRECATED = Cast<ADEPRECATED_ABlasterCharacter>(TryGetPawnOwner());
}

void UDEPRECATED_UBlasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (BlasterCharacter_DEPRECATED == nullptr)
	{
		BlasterCharacter_DEPRECATED = Cast<ADEPRECATED_ABlasterCharacter>(TryGetPawnOwner());
	}

	if (BlasterCharacter_DEPRECATED == nullptr)
	{
		return;
	}

	FVector Velocity = BlasterCharacter_DEPRECATED->GetVelocity();
	FVector LateralVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);
	Speed = LateralVelocity.Size();

	bIsInAir = BlasterCharacter_DEPRECATED->GetCharacterMovement()->IsFalling();
	bIsAccelerating = BlasterCharacter_DEPRECATED->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.0f;
	bWeaponEquipped = BlasterCharacter_DEPRECATED->IsWeaponEquipped();
	bIsCrouched = BlasterCharacter_DEPRECATED->bIsCrouched;
	bAiming = BlasterCharacter_DEPRECATED->IsAiming();
	EquippedWeapon = BlasterCharacter_DEPRECATED->GetEquippedWeapon();
	TurnInPlace = BlasterCharacter_DEPRECATED->GetTurnInPlaceDirection();
	bRotateRootBone = BlasterCharacter_DEPRECATED->ShouldRotateRootBone();

	// Offset yaw for strafing
	FRotator AimRotation = BlasterCharacter_DEPRECATED->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter_DEPRECATED->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 6.0f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter_DEPRECATED->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaSeconds;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaSeconds, 6.0f);
	Lean = FMath::Clamp(Interp, -90.0f, 90.0f);

	AO_Yaw = BlasterCharacter_DEPRECATED->GetAOYaw();
	AO_Pitch = BlasterCharacter_DEPRECATED->GetAOPitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter_DEPRECATED->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		BlasterCharacter_DEPRECATED->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (BlasterCharacter_DEPRECATED->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), RTS_World);
			FRotator LookAtRotator = UKismetMathLibrary::FindLookAtRotation(BlasterCharacter_DEPRECATED->GetHitTarget(), RightHandTransform.GetLocation());
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotator, DeltaSeconds, 30.0f);
		}
	}
}
