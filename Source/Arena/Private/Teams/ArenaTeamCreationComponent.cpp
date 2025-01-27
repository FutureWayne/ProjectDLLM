// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/ArenaTeamCreationComponent.h"

#include "GameMode/ArenaGameMode.h"
#include "Player/ArenaPlayerState.h"
#include "Teams/ArenaTeamInfo.h"

UArenaTeamCreationComponent::UArenaTeamCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TeamInfoClass = AArenaTeamInfo::StaticClass();
}

#if WITH_EDITOR
EDataValidationResult UArenaTeamCreationComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	//@TODO: TEAMS: Validate that all display assets have the same properties set!

	return Result;
}
#endif

void UArenaTeamCreationComponent::BeginPlay()
{
	Super::BeginPlay();

#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateTeams();
		ServerAssignPlayersToTeams();
	}
#endif
}

#if WITH_SERVER_CODE

void UArenaTeamCreationComponent::ServerCreateTeams()
{
	for (uint8 TeamId : TeamsToCreate)
	{
		ServerCreateTeam(TeamId);
	}
}

void UArenaTeamCreationComponent::ServerAssignPlayersToTeams()
{
	// Assign players that already exist to teams
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AArenaPlayerState* ArenaPS = Cast<AArenaPlayerState>(PS))
		{
			ServerChooseTeamForPlayer(ArenaPS);
		}
	}

	// Listen for new players logging in
	AArenaGameMode* GameMode = Cast<AArenaGameMode>(GameState->AuthorityGameMode);
	check(GameMode);

	GameMode->OnGameModePlayerInitialized.AddUObject(this, &ThisClass::OnPlayerInitialized);
}

void UArenaTeamCreationComponent::ServerChooseTeamForPlayer(AArenaPlayerState* PS)
{
	if (PS->IsOnlyASpectator())
	{
		PS->SetGenericTeamId(FGenericTeamId::NoTeam);
	}
	else
	{
		const FGenericTeamId TeamId = IntegerToGenericTeamId(GetLeastPopulatedTeamID());
		PS->SetGenericTeamId(TeamId);
	}
}

void UArenaTeamCreationComponent::OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer)
{
	check(NewPlayer);
	check(NewPlayer->PlayerState);

	if (AArenaPlayerState* ArenaPS = Cast<AArenaPlayerState>(NewPlayer->PlayerState))
	{
		ServerChooseTeamForPlayer(ArenaPS);
	}
}

void UArenaTeamCreationComponent::ServerCreateTeam(int32 TeamId)
{
	check(HasAuthority());

	// ensure the team doesn't already exist

	UWorld* World = GetWorld();
	check(World);

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AArenaTeamInfo* NewTeamInfo = World->SpawnActor<AArenaTeamInfo>(TeamInfoClass, SpawnInfo);
	checkf(NewTeamInfo != nullptr, TEXT("Failed to spawn team info"));
	NewTeamInfo->SetTeamId(TeamId);
}

int32 UArenaTeamCreationComponent::GetLeastPopulatedTeamID() const
{
	const int32 NumTeams = TeamsToCreate.Num();
	if (NumTeams == 0)
	{
		return INDEX_NONE;
	}

	TMap<int32, uint32> TeamMemberCounts;
	TeamMemberCounts.Reserve(NumTeams);

	for (uint8 TeamId : TeamsToCreate)
	{
		TeamMemberCounts.Add(TeamId, 0);
	}

	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (AArenaPlayerState* ArenaPS = Cast<AArenaPlayerState>(PS))
		{
			const int32 PlayerTeamId = ArenaPS->GetTeamId();

			if (((PlayerTeamId != INDEX_NONE) && !ArenaPS->IsInactive()))
			{
				check(TeamMemberCounts.Contains(PlayerTeamId));
				TeamMemberCounts[PlayerTeamId]++;
			}
		}
	}

	// Sort by lowest team population, then by team ID
	int32 BestTeamId = INDEX_NONE;
	uint32 LowestPlayerCount = TNumericLimits<uint32>::Max();

	for (const auto& KVP : TeamMemberCounts)
	{
		int32 TestTeamId = KVP.Key;
		uint32 TestTeamPlayerCount = KVP.Value;

		if (TestTeamPlayerCount < LowestPlayerCount)
		{
			BestTeamId = TestTeamId;
			LowestPlayerCount = TestTeamPlayerCount;
		}
		else if (TestTeamPlayerCount == LowestPlayerCount)
		{
			if ((TestTeamId < BestTeamId) || (BestTeamId == INDEX_NONE))
			{
				BestTeamId = TestTeamId;
				LowestPlayerCount = TestTeamPlayerCount;
			}
		}
	}

	return BestTeamId;
}
#endif	// WITH_SERVER_CODE
