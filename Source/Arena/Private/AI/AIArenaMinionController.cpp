// Copyright Ludens Studio. All Rights Reserved.


#include "AI/AIArenaMinionController.h"
#include <AI/AIArenaMinionCharacter.h>
#include "BehaviorTree/BlackboardComponent.h"

AAIArenaMinionController::AAIArenaMinionController()
{

}

void AAIArenaMinionController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const auto MinionCharacter = Cast<AAIArenaMinionCharacter>(InPawn);

	if (MinionCharacter)
	{
		RunBehaviorTree(MinionCharacter->BehaviorTreeAsset);
	}
}

void AAIArenaMinionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const auto AimActor = GetFocusOnActor();

	SetFocus(AimActor);

}

AActor* AAIArenaMinionController::GetFocusOnActor() const
{
	if (!GetBlackboardComponent()) return nullptr;

	return Cast<AActor>(GetBlackboardComponent()->GetValueAsObject(FocusOnKeyName));
}
