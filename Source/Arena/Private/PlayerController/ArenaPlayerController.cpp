// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/ArenaPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "GameMode/TeamsGameMode.h"
#include "Input/ArenaInputComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/ArenaPlayerState.h"
#include "UI/HUD/ArenaHUD.h"
#include "UI/Widget/AgentChooseWidget.h"
#include "UI/Widget/CharacterOverlay.h"

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

void AArenaPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	ArenaHUD = !ArenaHUD ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD.Get();
	if (ArenaHUD)
	{
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = FMath::FloorToInt(FMath::Fmod(CountdownTime, 60.f));

		if (ArenaHUD && ArenaHUD->OverlayWidget && ArenaHUD->OverlayWidget->MatchCountdownText)
		{
			FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			ArenaHUD->OverlayWidget->MatchCountdownText->SetText(FText::FromString(TimeString));
		}
	}
}

void AArenaPlayerController::SetHUDAgentChooseCountdown(float CountdownTime)
{
	ArenaHUD = !ArenaHUD ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD.Get();
	if (ArenaHUD && ArenaHUD->AgentChooseWidget && ArenaHUD->AgentChooseWidget->ChooseTimeCountdown)
	{
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = FMath::FloorToInt(FMath::Fmod(CountdownTime, 60.f));

		FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		ArenaHUD->AgentChooseWidget->ChooseTimeCountdown->SetText(FText::FromString(TimeString));
	}
}

void AArenaPlayerController::OnMatchStateSet(const FName NewMatchState)
{
	if (HasAuthority())
	{
		MatchState = NewMatchState;
	}
	
	if (NewMatchState == MatchState::WaitingToStart)
	{
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}
	else if (NewMatchState == MatchState::InProgress)
	{
		HandleMatchStart();
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
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

void AArenaPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaPlayerController, MatchState);
}

void AArenaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ArenaHUD = Cast<AArenaHUD>(GetHUD());
	ServerCheckMatchState();

	check(DefaultMappingContext);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AArenaPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
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


void AArenaPlayerController::SetHUDTime()
{
	float TimeLeft = 0.0f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = AgentChooseDuration - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = AgentChooseDuration + MatchDuration - GetServerTime() + LevelStartingTime;
	}
	
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart)
		{
			SetHUDAgentChooseCountdown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
	
}

void AArenaPlayerController::HandleMatchWaitingToStart()
{
	ArenaHUD = !ArenaHUD ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD.Get();

	if (ArenaHUD)
	{
		ArenaHUD->AddAgentChooseWidget();
	}
}

void AArenaPlayerController::HandleMatchStart()
{
	ArenaHUD = !ArenaHUD ? Cast<AArenaHUD>(GetHUD()) : ArenaHUD.Get();
	if (ArenaHUD)
	{
		ArenaHUD->AgentChooseWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AArenaPlayerController::ServerCheckMatchState_Implementation()
{
	ATeamsGameMode* TeamsGameMode = GetWorld()->GetAuthGameMode<ATeamsGameMode>();
	if (TeamsGameMode)
	{
		AgentChooseDuration = TeamsGameMode->AgentChoosingDuration;
		MatchDuration = TeamsGameMode->MatchDuration;
		LevelStartingTime = TeamsGameMode->LevelStartingTime;
		MatchState = TeamsGameMode->GetMatchState();
		ClientJoinMidGame(MatchState, AgentChooseDuration, MatchDuration, LevelStartingTime);

		ArenaHUD = Cast<AArenaHUD>(GetHUD());
		if (ArenaHUD && MatchState == MatchState::WaitingToStart)
		{
			ArenaHUD->AddAgentChooseWidget();
		}
	}
}

void AArenaPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float AgentChoose, float Match, float LevelStart)
{
	AgentChooseDuration = AgentChoose;
	MatchDuration = Match;
	LevelStartingTime = LevelStart;
	MatchState = StateOfMatch;
	OnMatchStateSet(StateOfMatch);

	ArenaHUD = Cast<AArenaHUD>(GetHUD());
	if (ArenaHUD && MatchState == MatchState::WaitingToStart)
	{
		ArenaHUD->AddAgentChooseWidget();
	}
}

void AArenaPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::WaitingToStart)
	{
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchStart();
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
	}
}
