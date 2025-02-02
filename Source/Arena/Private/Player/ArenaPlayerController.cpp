// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ArenaPlayerController.h"

#include "ArenaGameplayTags.h"
#include "ArenaLogChannel.h"
#include "EnhancedInputSubsystems.h"
#include "Equipment/ArenaQuickBarComponent.h"
#include "Input/ArenaInputComponent.h"
#include "Inventory/ArenaInventoryManagerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/ArenaPlayerState.h"

AArenaPlayerController::AArenaPlayerController(const FObjectInitializer& ObjectInitializer)
{
	InventoryManagerComponent = CreateDefaultSubobject<UArenaInventoryManagerComponent>(TEXT("InventoryManagerComponent"));
	QuickBarComponent = CreateDefaultSubobject<UArenaQuickBarComponent>(TEXT("QuickBarComponent"));
}

void AArenaPlayerController::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent())
	{
		ArenaASC->AbilityInputTagPressed(InputTag);
	}
}

void AArenaPlayerController::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent())
	{
		ArenaASC->AbilityInputTagReleased(InputTag);
	}
}

void AArenaPlayerController::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* ControlledPawn = GetPawn<APawn>();

	AController* Controller = ControlledPawn ? ControlledPawn->GetController() : nullptr;
	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			ControlledPawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			ControlledPawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void AArenaPlayerController::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* ControlledPawn = GetPawn<APawn>();

	if (!ControlledPawn)
	{
		return;
	}
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		ControlledPawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		ControlledPawn->AddControllerPitchInput(Value.Y);
	}
}

void AArenaPlayerController::Input_Crouch(const FInputActionValue& InputActionValue)
{
}

void AArenaPlayerController::Input_Jump(const FInputActionValue& InputActionValue)
{
}

void AArenaPlayerController::Input_Sprint(const FInputActionValue& InputActionValue)
{
}

void AArenaPlayerController::Input_Walk(const FInputActionValue& InputActionValue)
{
}

AArenaPlayerState* AArenaPlayerController::GetArenaPlayerState() const
{
	return CastChecked<AArenaPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UArenaAbilitySystemComponent* AArenaPlayerController::GetArenaAbilitySystemComponent() const
{
	const AArenaPlayerState* ArenaPS = GetArenaPlayerState();
	return ArenaPS ? ArenaPS->GetArenaAbilitySystemComponent() : nullptr;
}

void AArenaPlayerController::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	UE_LOG(LogArenaTeams, Error, TEXT("You can't set the team ID on a player controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AArenaPlayerController::GetGenericTeamId() const
{
	if (const IArenaTeamAgentInterface* PSWithTeamInterface = Cast<IArenaTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}

FOnArenaTeamIndexChangedDelegate* AArenaPlayerController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AArenaPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AArenaPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AArenaPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void AArenaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	check(DefaultMappingContext);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AArenaPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AArenaPlayerController, InventoryManagerComponent);
	DOREPLIFETIME(AArenaPlayerController, QuickBarComponent);
}

void AArenaPlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
}

void AArenaPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent())
	{
		ArenaASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}
	
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AArenaPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

void AArenaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
}

void AArenaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UArenaInputComponent* ArenaIC = CastChecked<UArenaInputComponent>(InputComponent);
	check(ArenaIC);

	if (ensureMsgf(ArenaIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UArenaInputComponent or a subclass of it.")))
	{
		ArenaIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased);

		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_LookMouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Jump, ETriggerEvent::Triggered, this, &ThisClass::Input_Jump, /*bLogIfNotFound=*/ false);
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Sprint, ETriggerEvent::Triggered, this, &ThisClass::Input_Sprint, /*bLogIfNotFound=*/ false);
		ArenaIC->BindNativeAction(InputConfig, ArenaGameplayTags::InputTag_Walk, ETriggerEvent::Triggered, this, &ThisClass::Input_Walk, /*bLogIfNotFound=*/ false);
	}
}

void AArenaPlayerController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void AArenaPlayerController::BroadcastOnPlayerStateChanged()
{
	// Unbind from the old player state, if any
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (IArenaTeamAgentInterface* PlayerStateTeamInterface = Cast<IArenaTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// Bind to the new player state, if any
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (IArenaTeamAgentInterface* PlayerStateTeamInterface = Cast<IArenaTeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// Broadcast the team change (if it really has)
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	LastSeenPlayerState = PlayerState;
}
