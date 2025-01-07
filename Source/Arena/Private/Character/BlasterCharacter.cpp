// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterCharacter.h"

#include "ArenaGameplayTags.h"
#include "BlasterComponent/CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Equipment/ArenaEquipmentManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/ArenaInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/ArenaPlayerState.h"
#include "PlayerController/ArenaPlayerController.h"
#include "PlayerStart/TeamPlayerStart.h"
#include "Weapon/Weapon.h"

// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	DefaultJogSpeed = 500.0f;
	DefaultWalkSpeed = 250.0f;
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultJogSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	CameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_NotTurning;

	EquipmentManagerComponent = CreateDefaultSubobject<UArenaEquipmentManagerComponent>(TEXT("EquipmentManagerComponent"));
	
	SetNetUpdateFrequency(66.0f);
	SetMinNetUpdateFrequency(33.0f);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Register properties to replicate
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;
	
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return (Combat && Combat->EquippedWeapon != nullptr);
}

bool ABlasterCharacter::IsAiming() const
{
	return (Combat && Combat->bIsAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	if (Combat)
	{
		return Combat->EquippedWeapon;
	}

	return nullptr;
}

void ABlasterCharacter::PlayFireMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
	}
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (Combat == nullptr)
	{
		return FVector::ZeroVector;
	}
	return Combat->HitTarget;
}

void ABlasterCharacter::SetTeamColor(const ETeam Team) const
{
	if (GetMesh() == nullptr) return;

	switch (Team)
	{
	case ETeam::ET_Attack:
		GetMesh()->SetMaterial(0, RedMaterial1);
		GetMesh()->SetMaterial(1, RedMaterial2);
		break;
	case ETeam::ET_Defense:
		GetMesh()->SetMaterial(0, BlueMaterial1);
		GetMesh()->SetMaterial(1, BlueMaterial2);
		break;
	case ETeam::ET_Neutral:
		GetMesh()->SetMaterial(0, BlueMaterial1);
		GetMesh()->SetMaterial(1, BlueMaterial2);
		break;
	default:
		break;
	}
}

void ABlasterCharacter::SetSpawnPoint()
{
	if (HasAuthority() && ArenaPlayerState->GetTeam() != ETeam::ET_Max)
	{
		TArray<AActor*> SpawnPoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATeamPlayerStart::StaticClass(), SpawnPoints);

		TArray<ATeamPlayerStart*> TeamSpawnPoints;
		for (auto SpawnPoint : SpawnPoints)
		{
			ATeamPlayerStart* TeamSpawnPoint = Cast<ATeamPlayerStart>(SpawnPoint);
			if (TeamSpawnPoint && TeamSpawnPoint->Team == ArenaPlayerState->GetTeam())
			{
				TeamSpawnPoints.Add(TeamSpawnPoint);
			}
		}

		if (TeamSpawnPoints.Num() > 0)
		{
			const int32 RandomIndex = FMath::RandRange(0, TeamSpawnPoints.Num() - 1);
			SetActorLocation(TeamSpawnPoints[RandomIndex]->GetActorLocation());
			SetActorRotation(TeamSpawnPoints[RandomIndex]->GetActorRotation());
		}
	}
}

USkeletalMeshComponent* ABlasterCharacter::GetDisplayMesh_Implementation()
{
	return GetMesh();
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABlasterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaSeconds);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaSeconds;
		if (TimeSinceLastMovementReplication >= 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAOPitch();
	}
	
	//HideCharacterIfCameraClose();

	if (ArenaPlayerState == nullptr)
	{
		if (Cast<AArenaPlayerState>(GetPlayerState()))
		{
			OnPlayerStateInitialized();
		}
	}
	
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		ServerEquipButtonPressed();
	}
}

void ABlasterCharacter::DropButtonPressed()
{
	if (Combat)
	{
		ServerDropButtonPressed();
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void ABlasterCharacter::CalculateAOPitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360] to [-90, 0]
		AO_Pitch = FMath::Clamp(AO_Pitch, 270.f, 360.f) - 360.f;
	}
}

void ABlasterCharacter::AimOffset(float DeltaSeconds)
{
	if (Combat && Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	const float Speed = Velocity.Size();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.0f && !bIsInAir)
	{
		bRotateRootBone = true;
		const FRotator CurrentAimLocation = FRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		const FRotator DeltaAimLocation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimLocation, StartingAimRotation);
		AO_Yaw = DeltaAimLocation.Yaw;
		if (TurnInPlaceDirection == ETurnInPlaceDirection::ETIP_NotTurning)
		{
			Interp_AO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaSeconds);
	}

	if (Speed > 0.0f || bIsInAir)
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		AO_Yaw = 0.0f;
		bUseControllerRotationYaw = true;
		TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_NotTurning;
	}

	CalculateAOPitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)
	{
		return;
	}

	bRotateRootBone = false;

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame);
	ProxyYawDelta = Delta.Yaw;
	
	if (FMath::Abs(ProxyYawDelta) > TurnThreshold)
	{
		if (ProxyYawDelta > TurnThreshold)
		{
			TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_Right;
		}
		else if (ProxyYawDelta < -TurnThreshold)
		{
			TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_Left;
		}
		else
		{
			TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_NotTurning;
		}

		return;
	}

	TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_NotTurning;
}

void ABlasterCharacter::OnPlayerStateInitialized()
{
	Super::OnPlayerStateInitialized();

	SetTeamColor(ArenaPlayerState->GetTeam());
	SetSpawnPoint();
}

void ABlasterCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::WalkButtonPressed()
{
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
}

void ABlasterCharacter::WalkButtonReleased()
{
	GetCharacterMovement()->MaxWalkSpeed = DefaultJogSpeed;
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UArenaInputComponent* ArenaIC = Cast<UArenaInputComponent>(PlayerInputComponent);
	AArenaPlayerController* ArenaPC = Cast<AArenaPlayerController>(GetController());

	check(ArenaIC);
	check(ArenaPC);

	if (const UArenaInputConfig* InputConfig = ArenaPC->GetInputConfig())
	{
		// // Moving
		// ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move, false);
		//
		// // Jumping
		// ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Jump, ETriggerEvent::Started, this, &ACharacter::Jump, false);
		// ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping, false);
		//
		// // Looking
		// ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Look, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look, false);

		// Equipping
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Equip, ETriggerEvent::Triggered, this, &ABlasterCharacter::EquipButtonPressed, false);

		// Dropping
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Drop, ETriggerEvent::Triggered, this, &ABlasterCharacter::DropButtonPressed, false);

		// // Crouching
		// ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ABlasterCharacter::CrouchButtonPressed, false);

		// Aiming
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Aim, ETriggerEvent::Started, this, &ABlasterCharacter::AimButtonPressed, false);
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Aim, ETriggerEvent::Completed, this, &ABlasterCharacter::AimButtonReleased, false);

		// Firing
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Fire, ETriggerEvent::Started, this, &ABlasterCharacter::FireButtonPressed, false);
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Fire, ETriggerEvent::Completed, this, &ABlasterCharacter::FireButtonReleased, false);
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->OwningCharacter = this;
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f;
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}

	StartingAimRotation = FRotator(0.0f, GetControlRotation().Yaw, 0.0f);
}

void ABlasterCharacter::ServerDropButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->DropWeapon();
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(const AWeapon* LastWeapon) const
{
	// OverlappingWeapon has changed
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::OnRep_Health()
{
}

void ABlasterCharacter::TurnInPlace(float DeltaSeconds)
{
	if (AO_Yaw > 90.0f)
	{
		TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_Right;
	}
	else if (AO_Yaw < -90.0f)
	{
		TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_Left;
	}

	if (TurnInPlaceDirection != ETurnInPlaceDirection::ETIP_NotTurning)
	{
		Interp_AO_Yaw = FMath::FInterpTo(Interp_AO_Yaw, 0.f, DeltaSeconds, 5.0f);
		AO_Yaw = Interp_AO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurnInPlaceDirection = ETurnInPlaceDirection::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.0f, GetControlRotation().Yaw, 0.0f);
		}
	}
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if ((CameraComponent->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

