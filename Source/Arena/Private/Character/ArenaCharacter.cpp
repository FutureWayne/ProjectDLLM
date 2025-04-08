// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ArenaCharacter.h"

#include "ArenaLogChannels.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaCombatSet.h"
#include "Camera/ArenaCameraComponent.h"
#include "Character/ArenaHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Equipment/ArenaEquipmentManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/ArenaPlayerState.h"
#include "UI/HUD/ArenaHUD.h"


AArenaCharacter::AArenaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	HealthComponent = CreateDefaultSubobject<UArenaHealthComponent>(TEXT("HealthComponent"));
	
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	CameraComponent = CreateDefaultSubobject<UArenaCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));
	
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	EquipmentManagerComponent = CreateDefaultSubobject<UArenaEquipmentManagerComponent>(TEXT("EquipmentManagerComponent"));
	
	SetNetUpdateFrequency(66.0f);
	SetMinNetUpdateFrequency(33.0f);
}

UArenaAbilitySystemComponent* AArenaCharacter::GetArenaAbilitySystemComponent() const
{
	return Cast<UArenaAbilitySystemComponent>(AbilitySystemComponent);
}

UAbilitySystemComponent* AArenaCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void AArenaCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (CameraComponent)
	{
		CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
	}
}

// Called every frame
void AArenaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AArenaCharacter::PossessedBy(AController* NewController)
{
	const FGenericTeamId OldTeamId = GetGenericTeamId();
	
	Super::PossessedBy(NewController);

	// Grab the current team ID and listen for future changes
	if (IArenaTeamAgentInterface* ControllerAsTeamProvider = Cast<IArenaTeamAgentInterface>(NewController))
	{
		MyTeamId = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
	}
	ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamId);

	// Init ability actor info for the server
	InitAbilityActorInfo();
}

void AArenaCharacter::UnPossessed()
{
	AController* const OldController = Controller;

	// Stop listening for changes from the old controller
	const FGenericTeamId OldTeamId = MyTeamId;
	if (IArenaTeamAgentInterface* ControllerAsTeamProvider = Cast<IArenaTeamAgentInterface>(OldController))
	{
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}
	
	Super::UnPossessed();

	// Determine what the new team ID should be afterwards
	MyTeamId = DetermineNewTeamAfterPossessionEnds(OldTeamId);
	ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamId);
}

void AArenaCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, MyTeamId);
}

void AArenaCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	HealthComponent->DamageSelfDestruct(/*bFellOutOfWorld=*/ true);
}

void AArenaCharacter::NotifyControllerChanged()
{
	const FGenericTeamId OldTeamId = GetGenericTeamId();
	
	Super::NotifyControllerChanged();

	// Update our team ID based on the controller
	if (HasAuthority() && (Controller != nullptr))
	{
		if (IArenaTeamAgentInterface* ControllerWithTeam = Cast<IArenaTeamAgentInterface>(Controller))
		{
			MyTeamId = ControllerWithTeam->GetGenericTeamId();
			ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamId);
		}
	}
}

void AArenaCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	if (GetController() == nullptr)
	{
		if (HasAuthority())
		{
			const FGenericTeamId OldTeamId = MyTeamId;
			MyTeamId = NewTeamID;
			ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamId);
		}
		else
		{
			UE_LOG(LogArenaTeams, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
		}
	}
	else
	{
		UE_LOG(LogArenaTeams, Error, TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"), *GetPathName(this));
	}
}

FGenericTeamId AArenaCharacter::GetGenericTeamId() const
{
	return MyTeamId;
}

FOnArenaTeamIndexChangedDelegate* AArenaCharacter::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AArenaCharacter::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	const FGenericTeamId OldTeamId = MyTeamId;
	MyTeamId = IntegerToGenericTeamId(NewTeam);
	ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamId);
}

void AArenaCharacter::OnRep_MyTeamId(FGenericTeamId OldTeamId)
{
	ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamId);
}

void AArenaCharacter::OnDeathStarted(AActor* OwningActor)
{
	DisableMovementAndCollision();
}

void AArenaCharacter::OnDeathFinished(AActor* OwningActor)
{
	DestroyDueToDeath();
}

void AArenaCharacter::OnSpeedBuffChanged(float OldValue, float NewValue, AActor* EffectInstigator)
{
	K2_OnSpeedBuffChanged(OldValue, NewValue, EffectInstigator);
}

USkeletalMeshComponent* AArenaCharacter::GetDisplayMesh_Implementation()
{
	return GetMesh();
}

void AArenaCharacter::SetWantsToSprint(bool bNewWantsToSprint)
{
	K2_OnChangeWantsToSprint(bNewWantsToSprint);
}

void AArenaCharacter::SetWantsToWalk(bool bNewWantsToWalk)
{
	K2_OnChangeWantsToWalk(bNewWantsToWalk);
}

void AArenaCharacter::SetWantsToAim(bool bNewWantsToAim)
{
	K2_OnChangeWantsToAim(bNewWantsToAim);
}

void AArenaCharacter::SetAbilityCameraMode(TSubclassOf<UArenaCameraMode> CameraMode,
	const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	}
}

void AArenaCharacter::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	}
}

void AArenaCharacter::DisableMovementAndCollision() const
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	check(MovementComp);
	MovementComp->StopMovementImmediately();
	MovementComp->DisableMovement();
}

void AArenaCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();
	
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	SetActorHiddenInGame(true);
}

TSubclassOf<UArenaCameraMode> AArenaCharacter::DetermineCameraMode()
{
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}
	
	return DefaultCameraMode;
}

void AArenaCharacter::InitAbilityActorInfo()
{
	AArenaPlayerState* PS = GetPlayerState<AArenaPlayerState>();
	if (PS == nullptr)
	{
		return;
	}

	// PlayerState logically owns the AbilitySystemComponent and AttributeSet while the Character is the physical actor
	PS->GetAbilitySystemComponent()->InitAbilityActorInfo(PS, this);
	AbilitySystemComponent = PS->GetAbilitySystemComponent();
	ArenaHealthSet = PS->GetArenaHealthSet();
	ArenaCombatSet = PS->GetArenaCombatSet();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AArenaHUD* HUD = Cast<AArenaHUD>(PC->GetHUD()))
		{
			HUD->AddOverlayWidget();
		}
	}

	UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent();
	HealthComponent->InitializeWithAbilitySystem(ArenaASC);

	ArenaCombatSet.Get()->OnSpeedBuffChanged.AddDynamic(this, &ThisClass::OnSpeedBuffChanged);

	check(AbilitySet);
	PS->AddAbilitySet(AbilitySet);

	K2_OnAbilityActorInfoInitialized();
}

void AArenaCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
}
