#include "AI/BTServices/BTService_FindNearestTarget.h"
#include "AIController.h"
#include "BehaviorTree/BTFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include <Character/BlasterCharacter.h>
#include <Player/ArenaPlayerState.h>
#include <PlayerController/ArenaPlayerController.h>
#include "BehaviorTree/BlackboardComponent.h"
#include <AI/AIArenaMinionCharacter.h>
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/ArenaHealthComponent.h"

void UBTService_FindNearestTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	APawn* OwningPawn = OwnerComp.GetAIOwner()->GetPawn();

	AAIArenaMinionCharacter* AICharacter = Cast<AAIArenaMinionCharacter>(OwningPawn);

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlasterCharacter::StaticClass(), FoundActors);
	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosestActor = nullptr;
	for (AActor* Actor : FoundActors)
	{
		AArenaCharacter* ArenaActor = Cast<AArenaCharacter>(Actor);

		UArenaHealthComponent* ActorHealthComponent = UArenaHealthComponent::FindHealthComponent(Actor);
		if (!ActorHealthComponent || ActorHealthComponent->IsDeadOrDying())
		{
			continue;
		}
		
		// Check if the other actor is in the defense team
		const AController* Controller = ArenaActor->GetController();
		if (!Controller || !Controller->PlayerState)
		{
			continue;
		}

		if (AArenaPlayerState* ArenaPlayerState = Cast<AArenaPlayerState>(Controller->PlayerState))
		{
			ETeam ArenaPlayerTeam = ArenaPlayerState->GetTeam();
			if (ArenaPlayerTeam != ETeam::ET_Defense)
			{
				continue;
			}
		}

		//Find the closest enemy
		const float Distance = FVector::Dist(OwningPawn->GetActorLocation(), Actor->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestActor = Actor;
		}
	}

	float DetectRange = 0.0f;
	AActor* TowerActor = nullptr;
	if (AICharacter) 
	{
		DetectRange = AICharacter->DetectRangeRadius;
		TowerActor = AICharacter->TowerTarget;
	}

	if (ClosestDistance > DetectRange)
	{
		if (TowerActor) 
		{
			ClosestActor = TowerActor;
			ClosestDistance = FVector::Dist(OwningPawn->GetActorLocation(), TowerActor->GetActorLocation());
		}
		else 
		{
			ClosestActor = nullptr;
			ClosestDistance = TNumericLimits<float>::Max();
		}
	}

	const auto Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard)
	{
		Blackboard->SetValueAsObject(TargetToFollowSelector.SelectedKeyName, ClosestActor);
		if (ClosestActor) 
		{
			Blackboard->SetValueAsVector(MoveToLocation.SelectedKeyName, ClosestActor->GetActorLocation());
		}
		else
		{
			Blackboard->SetValueAsVector(MoveToLocation.SelectedKeyName, OwningPawn->GetActorLocation());
		}
		Blackboard->SetValueAsFloat(DistanceToTargetSelector.SelectedKeyName, ClosestDistance);
	}
}