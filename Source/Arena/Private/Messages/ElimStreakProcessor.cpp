// Copyright Ludens Studio. All Rights Reserved.

#include "Messages/ElimStreakProcessor.h"

#include "GameFramework/PlayerState.h"
#include "Messages/ArenaVerbMessage.h"
#include "Messages/ArenaVerbMessageHelpers.h"
#include "NativeGameplayTags.h"
#include "Teams/ArenaTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ElimStreakProcessor)

namespace ElimStreak
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_Elimination_Message, "Arena.Elimination.Message");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_Elimination_Feed, "Arena.Elimination.Feed");
}

void UElimStreakProcessor::StartListening()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		AddListenerHandle(MessageSubsystem.RegisterListener(ElimStreak::TAG_Arena_Elimination_Message, this, &ThisClass::OnEliminationMessage));
	}
}

void UElimStreakProcessor::OnEliminationMessage(FGameplayTag Channel, const FArenaVerbMessage& Payload)
{
	APlayerState* InstigatorPS = UArenaVerbMessageHelpers::GetPlayerStateFromObject(Payload.Instigator);
	APlayerState* TargetPS = UArenaVerbMessageHelpers::GetPlayerStateFromObject(Payload.Target);
	const UArenaTeamSubsystem* Teams = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
	const EArenaTeamComparison TeamComparison = Teams ? Teams->CompareTeams(InstigatorPS, TargetPS) : EArenaTeamComparison::InvalidArgument;
	const bool bEnemyElimination = InstigatorPS && InstigatorPS != TargetPS && TeamComparison == EArenaTeamComparison::DifferentTeams;

	if (InstigatorPS)
	{
		auto HandleStreak = [this, &Payload, InstigatorPS](TMap<TObjectPtr<APlayerState>, int32>& History,
			const TMap<int32, FGameplayTag>& StreakTags, int32 MaxStreak)
		{
			int32& StreakCount = History.FindOrAdd(InstigatorPS);
			StreakCount = FMath::Clamp(StreakCount + 1, 0, MaxStreak);
			if (const FGameplayTag* Tag = StreakTags.Find(StreakCount))
			{
				FArenaVerbMessage StreakMessage;
				StreakMessage.Verb = *Tag;
				StreakMessage.Instigator = InstigatorPS;
				StreakMessage.InstigatorTags = Payload.InstigatorTags;
				StreakMessage.ContextTags = Payload.ContextTags;
				StreakMessage.Magnitude = StreakCount;
				UGameplayMessageSubsystem::Get(this).BroadcastMessage(StreakMessage.Verb, StreakMessage);
			}
		};

		if (InstigatorPS == TargetPS)
		{
			HandleStreak(PlayerSelfElimHistory, SelfElimStreakTags, 10);
		}
		else if (bEnemyElimination)
		{
			HandleStreak(PlayerEnemyElimHistory, EnemyElimStreakTags, 6);
		}
		else if (TeamComparison == EArenaTeamComparison::OnSameTeam)
		{
			HandleStreak(PlayerFriendlyElimHistory, FriendlyElimStreakTags, 30);
		}
	}

	if (TargetPS)
	{
		PlayerEnemyElimHistory.Remove(TargetPS);

		// The feed shares this authoritative history with accolades, including death resets.
		int32 FeedCount = 0;
		if (InstigatorPS == TargetPS)
		{
			FeedCount = PlayerSelfElimHistory.FindRef(InstigatorPS);
		}
		else if (bEnemyElimination)
		{
			FeedCount = PlayerEnemyElimHistory.FindRef(InstigatorPS);
		}
		else if (InstigatorPS && TeamComparison == EArenaTeamComparison::OnSameTeam)
		{
			FeedCount = PlayerFriendlyElimHistory.FindRef(InstigatorPS);
		}

		FArenaVerbMessage FeedMessage = Payload;
		FeedMessage.Verb = ElimStreak::TAG_Arena_Elimination_Feed;
		FeedMessage.Instigator = InstigatorPS;
		FeedMessage.Target = TargetPS;
		FeedMessage.Magnitude = FeedCount;
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(FeedMessage.Verb, FeedMessage);
	}
}
