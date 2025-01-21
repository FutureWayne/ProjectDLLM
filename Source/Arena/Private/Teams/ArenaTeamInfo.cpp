// Copyright Ludens Studio. All Rights Reserved.


#include "Teams/ArenaTeamInfo.h"

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

	DOREPLIFETIME_CONDITION(ThisClass, Team, COND_InitialOnly);
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
	if (Team != ETeam::ET_Max)
	{
		UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			RegisterWithTeamSubsystem(TeamSubsystem);
		}
	}
}

void AArenaTeamInfo::SetTeam(const ETeam NewTeam)
{
	check(HasAuthority());
	check(Team == ETeam::ET_Max);
	check(NewTeam != ETeam::ET_Max);

	Team = NewTeam;

	TryRegisterWithTeamSubsystem();
}

void AArenaTeamInfo::OnRep_Team()
{
	TryRegisterWithTeamSubsystem();
}
