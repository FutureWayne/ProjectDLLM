// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/ArenaPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Input/ArenaInputComponent.h"
#include "Player/ArenaPlayerState.h"

void AArenaPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
}

void AArenaPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
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
