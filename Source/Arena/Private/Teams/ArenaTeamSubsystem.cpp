// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/ArenaTeamSubsystem.h"

#include "AbilitySystemGlobals.h"
#include "Player/ArenaPlayerState.h"

UArenaTeamSubsystem::UArenaTeamSubsystem()
{
}

void UArenaTeamSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();

	// Iterate ETeam enum and spawn a team info actor for each team
	for (uint8 i = 0; i < static_cast<uint8>(ETeam::ET_Max); ++i)
	{
		ETeam Team = static_cast<ETeam>(i);
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AArenaTeamInfo* TeamInfo = World->SpawnActor<AArenaTeamInfo>(AArenaTeamInfo::StaticClass(), SpawnInfo);
		if (TeamInfo)
		{
			TeamInfo->SetTeam(Team);
			RegisterTeamInfo(TeamInfo);
		}
	}
}

void UArenaTeamSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

bool UArenaTeamSubsystem::RegisterTeamInfo(AArenaTeamInfo* TeamInfo)
{
	if (!ensure(TeamInfo))
	{
		return false;
	}

	const ETeam TeamId = TeamInfo->GetTeam();
	if (ensure(TeamId != ETeam::ET_Max))
	{
		TeamMap.Add(TeamId, TeamInfo);
		return true;
	}
	
	return false;
}

bool UArenaTeamSubsystem::UnregisterTeamInfo(AArenaTeamInfo* TeamInfo)
{
	if (!ensure(TeamInfo))
	{
		return false;
	}

	const ETeam TeamId = TeamInfo->GetTeam();
	if (ensure(TeamId != ETeam::ET_Max))
	{
		TeamMap.Remove(TeamId);
		return true;
	}

	return false;
}

ETeam UArenaTeamSubsystem::FindTeamFromObject(const UObject* TestObject) const
{
	// See if it's directly a team agent
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(TestObject))
	{
		return GenericTeamIdToTeam(TeamAgent->GetGenericTeamId());
	}

	if (const AActor* TestActor = Cast<AActor>(TestObject))
	{
		// See if the instigator is a team actor
		if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(TestActor->GetInstigator()))
		{
			return GenericTeamIdToTeam(TeamAgent->GetGenericTeamId());
		}

		// TeamInfo actors don't actually have the team interface, so they need a special case
		if (const AArenaTeamInfo* TeamInfo = Cast<AArenaTeamInfo>(TestActor))
		{
			return TeamInfo->GetTeam();
		}
		
		if (const AArenaPlayerState* ArenaPS = FindPlayerStateFromActor(TestActor))
		{
			return ArenaPS->GetTeam();
		}
	}

	return ETeam::ET_Max;
}

const AArenaPlayerState* UArenaTeamSubsystem::FindPlayerStateFromActor(const AActor* PossibleTeamActor) const
{
	if (PossibleTeamActor != nullptr)
	{
		if (const APawn* Pawn = Cast<APawn>(PossibleTeamActor))
		{
			if (AArenaPlayerState* ArenaPS = Pawn->GetPlayerState<AArenaPlayerState>())
			{
				return ArenaPS;
			}
		}
		else if (const AController* PC = Cast<const AController>(PossibleTeamActor))
		{
			if (AArenaPlayerState* ArenaPS = PC->GetPlayerState<AArenaPlayerState>())
			{
				return ArenaPS;
			}
		}
		else if (const AArenaPlayerState* ArenaPS = Cast<const AArenaPlayerState>(PossibleTeamActor))
		{
			return ArenaPS;
		}
	}

	return nullptr;
}

bool UArenaTeamSubsystem::ChangeTeamForActor(AActor* ActorToChange, const ETeam NewTeam) const
{
	if (AArenaPlayerState* ArenaPS = const_cast<AArenaPlayerState*>(FindPlayerStateFromActor(ActorToChange)))
	{
		ArenaPS->SetGenericTeamId(TeamToGenericTeamId(NewTeam));
		return true;
	}
	else if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(ActorToChange))
	{
		TeamAgent->SetGenericTeamId(TeamToGenericTeamId(NewTeam));
		return true;
	}

	return false;
}

void UArenaTeamSubsystem::FindTeamFromActor(const UObject* TestObject, bool& bIsPartOfTeam, ETeam& Team) const
{
	Team = FindTeamFromObject(TestObject);
	bIsPartOfTeam = Team != ETeam::ET_Max;
}

EArenaTeamComparison UArenaTeamSubsystem::CompareTeams(const UObject* A, const UObject* B, ETeam& TeamA,
	ETeam& TeamB) const
{
	TeamA = FindTeamFromObject(A);
	TeamB = FindTeamFromObject(B);	

	if (TeamA == ETeam::ET_Max || TeamB == ETeam::ET_Max)
	{
		return EArenaTeamComparison::InvalidArgument;
	}
	else
	{
		return TeamA == TeamB ? EArenaTeamComparison::OnSameTeam : EArenaTeamComparison::DifferentTeams;
	}
}

EArenaTeamComparison UArenaTeamSubsystem::CompareTeams(const UObject* A, const UObject* B) const
{
	ETeam TeamA, TeamB;
	return CompareTeams(A, B, /*out*/ TeamA, /*out*/ TeamB);
}

bool UArenaTeamSubsystem::CanCauseDamage(const UObject* Instigator, const UObject* Target,
	bool bAllowDamageToSelf) const
{
	if (bAllowDamageToSelf)
	{
		if (Instigator == Target || FindPlayerStateFromActor(Cast<AActor>(Instigator)) == FindPlayerStateFromActor(Cast<AActor>(Target)))
		{
			return true;
		}
	}

	ETeam InstigatorTeam, TargetTeam;
	EArenaTeamComparison Comparison = CompareTeams(Instigator, Target, /*out*/ InstigatorTeam, /*out*/ TargetTeam);
	if (Comparison == EArenaTeamComparison::DifferentTeams)
	{
		return true;
	}
	else if ((Comparison == EArenaTeamComparison::InvalidArgument) && (InstigatorTeam != ETeam::ET_Max))
	{
		// Allow damaging non-team actors for now, as long as they have an ability system component
		return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<const AActor>(Target)) != nullptr;
	}

	return false;
}
