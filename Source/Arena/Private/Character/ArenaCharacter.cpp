// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ArenaCharacter.h"

#include "ArenaLogChannel.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaCombatSet.h"
#include "Camera/CameraComponent.h"
#include "Character/ArenaHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Equipment/ArenaEquipmentManagerComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
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

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	CameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

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

USkeletalMeshComponent* AArenaCharacter::GetDisplayMesh_Implementation()
{
	return GetMesh();
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

	ArenaCombatSet.Get()->OnSpeedBuffChanged.AddDynamic(this, &ThisClass::K2_OnSpeedBuffChanged);

	check(AbilitySet);
	PS->AddAbilitySet(AbilitySet);

	K2_OnAbilityActorInfoInitialized();
}

void AArenaCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitAbilityActorInfo();
}
