// Copyright Ludens Studio. All Rights Reserved.

#include "Messages/ElimStreakProcessor.h"

#include "GameFramework/PlayerState.h"
#include "Messages/ArenaVerbMessage.h"
#include "NativeGameplayTags.h"
#include "Teams/ArenaTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ElimStreakProcessor)

namespace ElimStreak
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_Elimination_Message, "Arena.Elimination.Message");
}

void UElimStreakProcessor::StartListening()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	AddListenerHandle(MessageSubsystem.RegisterListener(ElimStreak::TAG_Arena_Elimination_Message, this, &ThisClass::OnEliminationMessage));
}

void UElimStreakProcessor::OnEliminationMessage(FGameplayTag Channel, const FArenaVerbMessage& Payload)
{
    if (APlayerState* InstigatorPS = Cast<APlayerState>(Payload.Instigator))
    {
        auto HandleStreak = [this, &Payload](TMap<TObjectPtr<APlayerState>, int32>& History, const TMap<int32, FGameplayTag>& StreakTags, 
                                             APlayerState* PlayerPS, int32 MaxStreak) -> void
        {
            int32& StreakCount = History.FindOrAdd(PlayerPS);
            StreakCount = FMath::Clamp(++StreakCount, 0, MaxStreak);
            
            if (const FGameplayTag* Tag = StreakTags.Find(StreakCount))
            {
                FArenaVerbMessage StreakMessage;
                StreakMessage.Verb = *Tag;
                StreakMessage.Instigator = PlayerPS;
                StreakMessage.InstigatorTags = Payload.InstigatorTags;
                StreakMessage.ContextTags = Payload.ContextTags;
                StreakMessage.Magnitude = StreakCount;
                
                UGameplayMessageSubsystem::Get(this).BroadcastMessage(StreakMessage.Verb, StreakMessage);
            }
        };

        // Determine if this is a self elimination or a normal elimination.
        if (Payload.Instigator != Payload.Target)
        {
            if (UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>())
            {
                EArenaTeamComparison TeamComparison = TeamSubsystem->CompareTeams(InstigatorPS, Payload.Target);

                // Handle enemy elimination streak
                if (TeamComparison == EArenaTeamComparison::DifferentTeams)
                {
                    HandleStreak(PlayerEnemyElimHistory, EnemyElimStreakTags, InstigatorPS, 6);
                }
                
                // Handle friendly elimination streak
                else if (TeamComparison == EArenaTeamComparison::OnSameTeam)
                {
                    HandleStreak(PlayerFriendlyElimHistory, FriendlyElimStreakTags, InstigatorPS, 30);
                }
            }
        }
        else // Self-elimination branch: instigator is the target
        {
            HandleStreak(PlayerSelfElimHistory, SelfElimStreakTags, InstigatorPS, 10);
        }
    }

    // Always clear the enemy elimination streak for the target.
    if (APlayerState* TargetPS = Cast<APlayerState>(Payload.Target))
    {
        PlayerEnemyElimHistory.Remove(TargetPS);
    }
}


