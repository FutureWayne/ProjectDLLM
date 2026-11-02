// Copyright Ludens Studio. All Rights Reserved.


#include "GameState/ArenaGameState.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayEffectTypes.h"
#include "Messages/ArenaVerbMessage.h"
#include "Net/UnrealNetwork.h"


AArenaGameState::AArenaGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UArenaAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void AArenaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MatchFinaleData);
}

void AArenaGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AArenaGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(/*Owner=*/ this, /*Avatar=*/ this);
}


UAbilitySystemComponent* AArenaGameState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AArenaGameState::HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	const EArenaMatchFinaleStage Stage = ArenaMatchFinale::GetStageFromCueTag(GameplayCueTag);
	if (Stage == EArenaMatchFinaleStage::None)
	{
		IGameplayCueInterface::HandleGameplayCue(Self, GameplayCueTag, EventType, Parameters);
		return;
	}

	// The server swaps stage cues, which clients see as remove-then-add. The stage only ends with the finale data, so
	// removal is ignored here. A cue fires both an unreliable OnActive and a replicated WhileActive, and stages only
	// move forward, so anything at or behind the current stage is a duplicate or arrived out of order.
	if (EventType != EGameplayCueEvent::Removed && EventType != EGameplayCueEvent::Executed && Stage > LocalFinaleStage)
	{
		EnterLocalFinaleStage(Stage, Parameters.RawMagnitude);
	}
}

void AArenaGameState::SetMatchFinaleData(const FArenaMatchFinaleData& NewData)
{
	if (!HasAuthority())
	{
		return;
	}

	const bool bWasActive = MatchFinaleData.bActive;
	MatchFinaleData = NewData;
	ForceNetUpdate();

	if (bWasActive && !MatchFinaleData.bActive)
	{
		ClearLocalFinaleStage();
	}
}

void AArenaGameState::OnRep_MatchFinaleData(const FArenaMatchFinaleData& OldData)
{
	if (OldData.bActive && !MatchFinaleData.bActive)
	{
		ClearLocalFinaleStage();
	}
	else
	{
		TrySyncInitialFinaleTime();
	}
}

void AArenaGameState::OnRep_ReplicatedWorldTimeSecondsDouble()
{
	Super::OnRep_ReplicatedWorldTimeSecondsDouble();

	TrySyncInitialFinaleTime();
}

void AArenaGameState::EnterLocalFinaleStage(EArenaMatchFinaleStage NewStage, float StageSeconds)
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetRealTimeSeconds() : 0.0;
	const EArenaMatchFinaleStage OldStage = LocalFinaleStage;

	LocalFinaleStage = NewStage;
	LocalFinaleStageSeconds = FMath::Max(0.0f, StageSeconds);
	LocalFinaleStageStartRealTime = Now;
	bPendingInitialFinaleTimeSync = OldStage == EArenaMatchFinaleStage::None && NewStage > EArenaMatchFinaleStage::Result;
	TrySyncInitialFinaleTime();

	OnMatchFinaleStageChanged.Broadcast(NewStage, OldStage);
}

void AArenaGameState::TrySyncInitialFinaleTime()
{
	if (!bPendingInitialFinaleTimeSync || !MatchFinaleData.bActive || MatchFinaleData.TimingStage != LocalFinaleStage)
	{
		return;
	}

	// FastArray cue callbacks can run before the GameState's first time RepNotify. Wait for that sample and the
	// matching timestamp, in either arrival order. Later samples must not rebase this real-time clock after slow motion.
	if (!HasAuthority() && NumServerWorldTimeSecondsDeltas == 0)
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		const double ElapsedSeconds = FMath::Clamp(GetServerWorldTimeSeconds() - MatchFinaleData.StageStartServerTime,
			0.0, static_cast<double>(LocalFinaleStageSeconds));
		LocalFinaleStageStartRealTime = World->GetRealTimeSeconds() - ElapsedSeconds;
		bPendingInitialFinaleTimeSync = false;
	}
}

void AArenaGameState::ClearLocalFinaleStage()
{
	bPendingInitialFinaleTimeSync = false;

	if (LocalFinaleStage == EArenaMatchFinaleStage::None)
	{
		return;
	}

	const EArenaMatchFinaleStage OldStage = LocalFinaleStage;
	LocalFinaleStage = EArenaMatchFinaleStage::None;
	LocalFinaleStageSeconds = 0.0f;

	OnMatchFinaleStageChanged.Broadcast(EArenaMatchFinaleStage::None, OldStage);
}

float AArenaGameState::GetMatchFinaleStageElapsedSeconds() const
{
	const UWorld* World = GetWorld();
	if (LocalFinaleStage == EArenaMatchFinaleStage::None || World == nullptr)
	{
		return 0.0f;
	}

	return static_cast<float>(FMath::Max(0.0, World->GetRealTimeSeconds() - LocalFinaleStageStartRealTime));
}

float AArenaGameState::GetMatchFinaleStageRemainingSeconds() const
{
	return FMath::Max(0.0f, LocalFinaleStageSeconds - GetMatchFinaleStageElapsedSeconds());
}

float AArenaGameState::GetMatchFinaleStageProgress() const
{
	if (LocalFinaleStageSeconds <= UE_KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	return FMath::Clamp(GetMatchFinaleStageElapsedSeconds() / LocalFinaleStageSeconds, 0.0f, 1.0f);
}

void AArenaGameState::MulticastMessageToClients_Implementation(const FArenaVerbMessage Message)
{
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}

void AArenaGameState::MulticastReliableMessageToClients_Implementation(const FArenaVerbMessage Message)
{
	MulticastMessageToClients_Implementation(Message);
}

void AArenaGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnMatchFinaleStageChanged.Clear();

	Super::EndPlay(EndPlayReason);
}

void AArenaGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
}

void AArenaGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
}

void AArenaGameState::SeamlessTravelTransitionCheckpoint(bool bToTransitionMap)
{
	Super::SeamlessTravelTransitionCheckpoint(bToTransitionMap);
}

