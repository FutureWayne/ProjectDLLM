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

void AArenaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	check(DefaultMappingContext);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
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
