// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/ArenaTeamSubsystem.h"

#include "AbilitySystemGlobals.h"
#include "ArenaLogChannel.h"
#include "Player/ArenaPlayerState.h"
#include "Logging/LogMacros.h"

// Define and initialize the static const TMap
const TMap<int32, FLinearColor> UArenaTeamSubsystem::TeamColorMap = {
	{ 0, FLinearColor::Gray },
	{ 1, FLinearColor::Red },
	{ 2, FLinearColor::Blue }
};

UArenaTeamSubsystem::UArenaTeamSubsystem()
{
	
}

void UArenaTeamSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();
}

void UArenaTeamSubsystem::Deinitialize()
{
	// Destroy any existing TeamInfo actors
	UWorld* World = GetWorld();
	
	Super::Deinitialize();
}

bool UArenaTeamSubsystem::RegisterTeamInfo(AArenaTeamInfo* TeamInfo)
{
	if (!ensure(TeamInfo))
	{
		return false;
	}

	const int32 TeamId = TeamInfo->GetTeamId();
	if (ensure(TeamId != INDEX_NONE))
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

	const int32 TeamId = TeamInfo->GetTeamId();
	if (ensure(TeamId != INDEX_NONE))
	{
		TeamMap.Remove(TeamId);
		return true;
	}

	return false;
}

int32 UArenaTeamSubsystem::FindTeamFromObject(const UObject* TestObject) const
{
	// See if it's directly a team agent
	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(TestObject))
	{
		return GenericTeamIdToInteger(TeamAgent->GetGenericTeamId());
	}

	if (const AActor* TestActor = Cast<AActor>(TestObject))
	{
		// See if the instigator is a team actor
		if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(TestActor->GetInstigator()))
		{
			return GenericTeamIdToInteger(TeamAgent->GetGenericTeamId());
		}

		// TeamInfo actors don't actually have the team interface, so they need a special case
		if (const AArenaTeamInfo* TeamInfo = Cast<AArenaTeamInfo>(TestActor))
		{
			return TeamInfo->GetTeamId();
		}
		
		if (const AArenaPlayerState* ArenaPS = FindPlayerStateFromActor(TestActor))
		{
			return ArenaPS->GetTeamId();
		}
	}

	return INDEX_NONE;
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

bool UArenaTeamSubsystem::ChangeTeamForActor(AActor* ActorToChange, const int32 NewTeamId) const
{
	UE_LOG(LogArenaTeams, Log, TEXT("ChangeTeamForActor(%s, %d)"), *ActorToChange->GetPathName(), static_cast<uint8>(NewTeamId));
	
	if (AArenaPlayerState* ArenaPS = const_cast<AArenaPlayerState*>(FindPlayerStateFromActor(ActorToChange)))
	{
		ArenaPS->SetGenericTeamId(IntegerToGenericTeamId(NewTeamId));
		return true;
	}
	else if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(ActorToChange))
	{
		TeamAgent->SetGenericTeamId(IntegerToGenericTeamId(NewTeamId));
		return true;
	}

	return false;
}

void UArenaTeamSubsystem::FindTeamFromActor(const UObject* TestObject, bool& bIsPartOfTeam, int32& TeamId) const
{
	TeamId = FindTeamFromObject(TestObject);
	bIsPartOfTeam = TeamId != INDEX_NONE;
}

EArenaTeamComparison UArenaTeamSubsystem::CompareTeams(const UObject* A, const UObject* B, int32& TeamIdA,
	int32& TeamIdB) const
{
	TeamIdA = FindTeamFromObject(A);
	TeamIdB = FindTeamFromObject(B);	

	if (TeamIdA == INDEX_NONE || TeamIdB == INDEX_NONE)
	{
		return EArenaTeamComparison::InvalidArgument;
	}
	else
	{
		return TeamIdA == TeamIdB ? EArenaTeamComparison::OnSameTeam : EArenaTeamComparison::DifferentTeams;
	}
}

EArenaTeamComparison UArenaTeamSubsystem::CompareTeams(const UObject* A, const UObject* B) const
{
	int32 TeamIdA;
	int32 TeamIdB;
	return CompareTeams(A, B, /*out*/ TeamIdA, /*out*/ TeamIdB);
}

void UArenaTeamSubsystem::FindTeamFromObject(const UObject* Agent, bool& bIsPartOfTeam, int32& TeamId,
	FLinearColor& TeamColor, bool bLogIfNotSet)
{
	bIsPartOfTeam = false;
	TeamId = INDEX_NONE;
	TeamColor = FLinearColor::White;

	if (UWorld* World = GEngine->GetWorldFromContextObject(Agent, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UArenaTeamSubsystem* TeamSubsystem = World->GetSubsystem<UArenaTeamSubsystem>())
		{
			TeamId = (TeamSubsystem->FindTeamFromObject(Agent));
			if (TeamId != INDEX_NONE)
			{
				bIsPartOfTeam = true;

				TeamColor = TeamSubsystem->FindTeamColorFromActor(Agent);

				if ((TeamColor == FLinearColor::White) && bLogIfNotSet)
				{
					UE_LOG(LogArenaTeams, Log, TEXT("FindTeamFromObject(%s) called too early (found team %d but no display asset set yet"), *Agent->GetPathName(), TeamId);
				}
			}
		}
		else
		{
			UE_LOG(LogArenaTeams, Error, TEXT("FindTeamFromObject(%s) failed: Team subsystem does not exist yet"), *Agent->GetPathName());
		}
	}
}

FLinearColor UArenaTeamSubsystem::FindTeamColorFromActor(const UObject* TestObject) const
{
	int32 TeamId = FindTeamFromObject(TestObject);
	if (TeamId != INDEX_NONE)
	{
		if (const FLinearColor* TeamColor = TeamColorMap.Find(TeamId))
		{
			return *TeamColor;
		}
	}

	return FLinearColor::White;
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

	int32 InstigatorTeamId, TargetTeamId;
	EArenaTeamComparison Comparison = CompareTeams(Instigator, Target, /*out*/ InstigatorTeamId, /*out*/ TargetTeamId);
	if (Comparison == EArenaTeamComparison::DifferentTeams)
	{
		return true;
	}
	else if ((Comparison == EArenaTeamComparison::InvalidArgument) && (InstigatorTeamId != INDEX_NONE))
	{
		// Allow damaging non-team actors for now, as long as they have an ability system component
		return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<const AActor>(Target)) != nullptr;
	}

	return false;
}

void UArenaTeamSubsystem::AddTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount)
{
	auto FailureHandler = [&](const FString& ErrorMessage)
	{
		UE_LOG(LogArenaTeams, Error, TEXT("AddTeamTagStack(TeamId: %d, Tag: %s, StackCount: %d) %s"), TeamId, *Tag.ToString(), StackCount, *ErrorMessage);
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

	UE_LOG(LogArenaTeams, Log, TEXT("AddTeamTagStack(TeamId: %d, Tag: %s, StackCount: %d)"), TeamId, *Tag.ToString(), StackCount);
	TeamInfo->TeamTags.AddStack(Tag, StackCount);
}

void UArenaTeamSubsystem::RemoveTeamTagStack(int32 TeamId, FGameplayTag Tag, int32 StackCount)
{
	auto FailureHandler = [&](const FString& ErrorMessage)
	{
		UE_LOG(LogArenaTeams, Error, TEXT("RemoveTeamTagStack(TeamId: %d, Tag: %s, StackCount: %d) %s"), TeamId, *Tag.ToString(), StackCount, *ErrorMessage);
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

	UE_LOG(LogArenaTeams, Log, TEXT("RemoveTeamTagStack(TeamId: %d, Tag: %s, StackCount: %d)"), TeamId, *Tag.ToString(), StackCount);
	TeamInfo->TeamTags.RemoveStack(Tag, StackCount);
}

int32 UArenaTeamSubsystem::GetTeamTagStackCount(int32 TeamId, FGameplayTag Tag) const
{
	const AArenaTeamInfo* TeamInfo = TeamMap.FindRef(TeamId);
	if (TeamInfo)
	{
		int32 StackCount = TeamInfo->TeamTags.GetStackCount(Tag);
		return TeamInfo->TeamTags.GetStackCount(Tag);
	}
	else
	{
		return 0;
	}
}

bool UArenaTeamSubsystem::TeamHasTag(int32 TeamId, FGameplayTag Tag) const
{
	return GetTeamTagStackCount(TeamId, Tag) > 0;
}
