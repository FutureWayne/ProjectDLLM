// Copyright Ludens Studio. All Rights Reserved.


#include "Player/ArenaPlayerSpawningManagerComponent.h"

#include "EngineUtils.h"
#include "Engine/PlayerStartPIE.h"
#include "GameFramework/PlayerState.h"
#include "Player/ArenaPlayerStart.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaPlayerSpawningManagerComponent)

DEFINE_LOG_CATEGORY_STATIC(LogPlayerSpawning, Log, All);

UArenaPlayerSpawningManagerComponent::UArenaPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(false);
	bAutoRegister = true;
	bAutoActivate = true;
	bWantsInitializeComponent = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UArenaPlayerSpawningManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ThisClass::OnLevelAdded);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleOnActorSpawned));

	for (AArenaPlayerStart* PlayerStart : TActorRange<AArenaPlayerStart>(World))
	{
		CachedPlayerStarts.Add(PlayerStart);
	}
}

void UArenaPlayerSpawningManagerComponent::OnLevelAdded(ULevel* InLevel, UWorld* InWorld)
{
	if (InWorld == GetWorld())
	{
		for (AActor* Actor : InLevel->Actors)
		{
			if (AArenaPlayerStart* PlayerStart = Cast<AArenaPlayerStart>(Actor))
			{
				ensure(!CachedPlayerStarts.Contains(PlayerStart));
				CachedPlayerStarts.Add(PlayerStart);
			}
		}
	}
}

void UArenaPlayerSpawningManagerComponent::HandleOnActorSpawned(AActor* SpawnedActor)
{
	if (AArenaPlayerStart* PlayerStart = Cast<AArenaPlayerStart>(SpawnedActor))
	{
		CachedPlayerStarts.Add(PlayerStart);
	}
}

// AArenaGameMode Proxied Calls - Need to handle when someone chooses
// to restart a player the normal way in the engine.
//======================================================================

AActor* UArenaPlayerSpawningManagerComponent::ChoosePlayerStart(AController* Player)
{
	if (Player)
	{
#if WITH_EDITOR
		if (APlayerStart* PlayerStart = FindPlayFromHereStart(Player))
		{
			return PlayerStart;
		}
#endif

		TArray<AArenaPlayerStart*> StarterPoints;
		for (auto StartIt = CachedPlayerStarts.CreateIterator(); StartIt; ++StartIt)
		{
			if (AArenaPlayerStart* Start = (*StartIt).Get())
			{
				StarterPoints.Add(Start);
			}
			else
			{
				UE_LOG(LogPlayerSpawning, Warning, TEXT("ChoosePlayerStart: Found a null player start in the cached list. Removing."));
				StartIt.RemoveCurrent();
			}
		}

		if (APlayerState* PlayerState = Player->GetPlayerState<APlayerState>())
		{
			// start dedicated spectators at any random starting location, but they do not claim it
			if (PlayerState->IsOnlyASpectator())
			{
				if (!StarterPoints.IsEmpty())
				{
					return StarterPoints[FMath::RandRange(0, StarterPoints.Num() - 1)];
				}

				return nullptr;
			}
		}

		AActor* PlayerStart = OnChoosePlayerStart(Player, StarterPoints);

		if (!PlayerStart)
		{
			PlayerStart = GetFirstRandomUnoccupiedPlayerStart(Player, StarterPoints);
		}

		if (AArenaPlayerStart* ArenaStart = Cast<AArenaPlayerStart>(PlayerStart))
		{
			ArenaStart->TryClaim(Player);
		}
		
		return PlayerStart;
	}

	return nullptr;
}

#if WITH_EDITOR
APlayerStart* UArenaPlayerSpawningManagerComponent::FindPlayFromHereStart(AController* Player)
{
	// Only 'Play From Here' for a player controller, bots etc. should all spawn from normal spawn points.
	if (Player->IsA<APlayerController>())
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<APlayerStart> It(World); It; ++It)
			{
				if (APlayerStart* PlayerStart = *It)
				{
					if (PlayerStart->IsA<APlayerStartPIE>())
					{
						// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
						return PlayerStart;
					}
				}
			}
		}
	}

	return nullptr;
}
#endif



bool UArenaPlayerSpawningManagerComponent::ControllerCanRestart(AController* Player)
{
	bool bCanRestart = true;

	// TODO: Can they restart?
	
	return bCanRestart;
}

void UArenaPlayerSpawningManagerComponent::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	OnFinishRestartPlayer(NewPlayer, StartRotation);
	K2_OnFinishRestartPlayer(NewPlayer, StartRotation);
}

//================================================================

void UArenaPlayerSpawningManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


AArenaPlayerStart* UArenaPlayerSpawningManagerComponent::GetFirstRandomUnoccupiedPlayerStart(AController* Controller,
	const TArray<AArenaPlayerStart*>& FoundStartPoints) const
{
	if (Controller)
	{
		TArray<AArenaPlayerStart*> UnOccupiedStartPoints;
		TArray<AArenaPlayerStart*> PartialOccupiedStartPoints;

		for	(AArenaPlayerStart* Start : FoundStartPoints)
		{
			EArenaPlayerStartLocationOccupancy Occupancy = Start->GetLocationOccupancy(Controller);

			switch (Occupancy)
			{
				case EArenaPlayerStartLocationOccupancy::Empty:
					UnOccupiedStartPoints.Add(Start);
					break;
				case EArenaPlayerStartLocationOccupancy::Partial:
					PartialOccupiedStartPoints.Add(Start);
					break;
				default: ;
			}
		}

		if (UnOccupiedStartPoints.Num() > 0)
		{
			return UnOccupiedStartPoints[FMath::RandRange(0, UnOccupiedStartPoints.Num() - 1)];
		}
		else if (PartialOccupiedStartPoints.Num() > 0)
		{
			return PartialOccupiedStartPoints[FMath::RandRange(0, PartialOccupiedStartPoints.Num() - 1)];
		}
	}

	return nullptr;
}

