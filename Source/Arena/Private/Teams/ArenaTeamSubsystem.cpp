// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/ArenaTeamSubsystem.h"

#include "AbilitySystemGlobals.h"
#include "EngineUtils.h"
#include "Player/ArenaPlayerState.h"

// Define and initialize the static const TMap
const TMap<ETeam, FLinearColor> UArenaTeamSubsystem::TeamColorMap = {
	{ ETeam::ET_Neutral, FLinearColor::Gray },
	{ ETeam::ET_Attack, FLinearColor::Red },
	{ ETeam::ET_Defense, FLinearColor::Blue }
};

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
	// Destroy any existing TeamInfo actors
	UWorld* World = GetWorld();
	for (TActorIterator<AArenaTeamInfo> It(World); It; ++It)
	{
		AArenaTeamInfo* ExistingTeamInfo = *It;
		if (ExistingTeamInfo)
		{
			ExistingTeamInfo->Destroy();
		}
	}
	
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

void UArenaTeamSubsystem::FindTeamFromObject(const UObject* Agent, bool& bIsPartOfTeam, ETeam& TeamId,
	FLinearColor& TeamColor, bool bLogIfNotSet)
{
	bIsPartOfTeam = false;
	TeamId = ETeam::ET_Max;
	TeamColor = FLinearColor::White;

	if (UWorld* World = GEngine->GetWorldFromContextObject(Agent, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UArenaTeamSubsystem* TeamSubsystem = World->GetSubsystem<UArenaTeamSubsystem>())
		{
			TeamId = (TeamSubsystem->FindTeamFromObject(Agent));
			if (TeamId != ETeam::ET_Max)
			{
				bIsPartOfTeam = true;

				TeamColor = TeamSubsystem->FindTeamColorFromActor(Agent);

				if ((TeamColor == FLinearColor::White) && bLogIfNotSet)
				{
					UE_LOG(LogTemp, Log, TEXT("FindTeamFromObject(%s) called too early (found team %d but no display asset set yet"), *Agent->GetPathName(), TeamId);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("FindTeamFromObject(%s) failed: Team subsystem does not exist yet"), *Agent->GetPathName());
		}
	}
}

FLinearColor UArenaTeamSubsystem::FindTeamColorFromActor(const UObject* TestObject) const
{
	ETeam Team = FindTeamFromObject(TestObject);
	if (Team != ETeam::ET_Max)
	{
		if (const FLinearColor* TeamColor = TeamColorMap.Find(Team))
		{
			return *TeamColor;
		}
	}

	return FLinearColor::White;
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

void UArenaTeamSubsystem::AddTeamTagStack(ETeam TeamId, FGameplayTag Tag, int32 StackCount)
{
	auto FailureHandler = [&](const FString& ErrorMessage)
	{
		UE_LOG(LogTemp, Error, TEXT("AddTeamTagStack(TeamId: %d, Tag: %s, StackCount: %d) %s"), TeamId, *Tag.ToString(), StackCount, *ErrorMessage);
	};

	AArenaTeamInfo* TeamInfo = TeamMap.FindRef(TeamId);
	if (!ensure(TeamInfo))
	{
		FailureHandler(TEXT("failed to find team info"));
		return;
	}

	if (!TeamInfo->HasAuthority())
	{
		FailureHandler(TEXT("failed because it was called on a client"));
		return;
	}
	
	TeamInfo->TeamTags.AddStack(Tag, StackCount);
}

void UArenaTeamSubsystem::RemoveTeamTagStack(ETeam TeamId, FGameplayTag Tag, int32 StackCount)
{
	auto FailureHandler = [&](const FString& ErrorMessage)
	{
		UE_LOG(LogTemp, Error, TEXT("RemoveTeamTagStack(TeamId: %d, Tag: %s, StackCount: %d) %s"), TeamId, *Tag.ToString(), StackCount, *ErrorMessage);
	};

	AArenaTeamInfo* TeamInfo = TeamMap.FindRef(TeamId);
	if (!ensure(TeamInfo))
	{
		FailureHandler(TEXT("failed to find team info"));
		return;
	}

	if (!TeamInfo->HasAuthority())
	{
		FailureHandler(TEXT("failed because it was called on a client"));
		return;
	}

	TeamInfo->TeamTags.RemoveStack(Tag, StackCount);
}

int32 UArenaTeamSubsystem::GetTeamTagStackCount(ETeam TeamId, FGameplayTag Tag) const
{
	AArenaTeamInfo* TeamInfo = TeamMap.FindRef(TeamId);
	if (TeamInfo)
	{
		return TeamInfo->TeamTags.GetStackCount(Tag);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetTeamTagStackCount(TeamId: %d, Tag: %s) failed to find team info"), TeamId, *Tag.ToString());
		return 0;
	}
}

bool UArenaTeamSubsystem::TeamHasTag(ETeam TeamId, FGameplayTag Tag) const
{
	return GetTeamTagStackCount(TeamId, Tag) > 0;
}
