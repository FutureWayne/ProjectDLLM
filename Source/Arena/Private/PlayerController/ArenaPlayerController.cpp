// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/ArenaPlayerController.h"

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

void AArenaPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent())
	{
		ArenaASC->AbilityInputTagPressed(InputTag);
	}
}

void AArenaPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	if (UArenaAbilitySystemComponent* ArenaASC = GetArenaAbilitySystemComponent())
	{
		ArenaASC->AbilityInputTagReleased(InputTag);
	}
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

	UArenaInputComponent* ArenaInputComponent = CastChecked<UArenaInputComponent>(InputComponent);
	check(ArenaInputComponent);
	
	ArenaInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased);
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
