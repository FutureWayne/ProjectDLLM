// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/ArenaTeamInfo.h"

#include "ArenaLogChannel.h"
#include "Net/UnrealNetwork.h"
#include "Teams/ArenaTeamSubsystem.h"

AArenaTeamInfo::AArenaTeamInfo(const FObjectInitializer& ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetPriority = 3.0f;
	SetReplicatingMovement(false);
}

void AArenaTeamInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TeamId);
	DOREPLIFETIME(ThisClass, TeamTags);
}

void AArenaTeamInfo::BeginPlay()
{
	Super::BeginPlay();

	TryRegisterWithTeamSubsystem();
}

void AArenaTeamInfo::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AArenaTeamInfo::RegisterWithTeamSubsystem(UArenaTeamSubsystem* Subsystem)
{
	Subsystem->RegisterTeamInfo(this);
}

void AArenaTeamInfo::TryRegisterWithTeamSubsystem()
{
	if (TeamId != INDEX_NONE)
	{
		UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			RegisterWithTeamSubsystem(TeamSubsystem);
		}
	}
}

void AArenaTeamInfo::SetTeamId(const int32 NewTeamId)
{
	check(HasAuthority());
	check(TeamId == INDEX_NONE);
	check(NewTeamId != INDEX_NONE);

	TeamId = NewTeamId;

	TryRegisterWithTeamSubsystem();
}

void AArenaTeamInfo::OnRep_Team()
{
	TryRegisterWithTeamSubsystem();
}

void AArenaTeamInfo::OnRep_TeamTags()
{
	UE_LOG(LogArenaTeams, Warning, TEXT("Team tags replicated"));
}
