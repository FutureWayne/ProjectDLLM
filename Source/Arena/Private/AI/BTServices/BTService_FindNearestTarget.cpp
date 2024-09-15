#include "AI/BTServices/BTService_FindNearestTarget.h"
#include "AIController.h"
#include "BehaviorTree/BTFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include <Character/BlasterCharacter.h>
#include <Player/ArenaPlayerState.h>
#include <PlayerController/ArenaPlayerController.h>
#include "BehaviorTree/BlackboardComponent.h"

void UBTService_FindNearestTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	const APawn* OwningPawn = OwnerComp.GetAIOwner()->GetPawn();
	//const FName TargetTag = OwningPawn->ActorHasTag(FName("Radiant")) ? FName("Dire") : FName("Radiant");
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TargetTag: %s"), *TargetTag.ToString()));

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlasterCharacter::StaticClass(), FoundActors);

	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosestActor = nullptr;
	for (AActor* Actor : FoundActors)
	{
		AArenaCharacter* ArenaActor = Cast<AArenaCharacter>(Actor);
		UArenaHealthSet* ActorHealthSet = ArenaActor->GetArenaHealthSet();
		if (!ActorHealthSet)
		{
			continue;
		}
		const float Distance = FVector::Dist(OwningPawn->GetActorLocation(), Actor->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestActor = Actor;
		}
	}

	const auto Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard)
	{
		Blackboard->SetValueAsObject(TargetToFollowSelector.SelectedKeyName, ClosestActor);
		Blackboard->SetValueAsVector(MoveToLocation.SelectedKeyName, ClosestActor->GetActorLocation());
		Blackboard->SetValueAsFloat(DistanceToTargetSelector.SelectedKeyName, ClosestDistance);
	}
}