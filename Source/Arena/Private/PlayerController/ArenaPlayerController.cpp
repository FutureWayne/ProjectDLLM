// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/ArenaPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Input/ArenaInputComponent.h"
#include "Player/ArenaPlayerState.h"

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

void AArenaPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
}

AArenaPlayerController::AArenaPlayerController(const FObjectInitializer& ObjectInitializer)
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

void AArenaPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	const float TimeOfServerReceivedRequest = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfServerReceivedRequest, TimeOfClientRequest);
}

void AArenaPlayerController::ClientReportServerTime_Implementation(float TimeOfServerReceivedRequest,
	float TimeOfClientRequest)
{
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeOfServerReceivedRequest + RoundTripTime * 0.5f;
	ClientServerDeltaTime = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float AArenaPlayerController::GetServerTime()
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	else
	{
		return GetWorld()->GetTimeSeconds() + ClientServerDeltaTime;
	}
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

	// Sync with server time
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
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

void AArenaPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckTimeSync(DeltaSeconds);
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

void AArenaPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}
