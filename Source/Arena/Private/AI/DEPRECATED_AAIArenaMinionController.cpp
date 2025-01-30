// Copyright Ludens Studio. All Rights Reserved.


#include "AI/DEPRECATED_AAIArenaMinionController.h"
#include <AI/DEPRECATED_AAIArenaMinionCharacter.h>
#include "BehaviorTree/BlackboardComponent.h"

ADEPRECATED_AAIArenaMinionController::ADEPRECATED_AAIArenaMinionController()
{

}

void ADEPRECATED_AAIArenaMinionController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const auto MinionCharacter = Cast<ADEPRECATED_AAIArenaMinionCharacter>(InPawn);

	if (MinionCharacter)
	{
		RunBehaviorTree(MinionCharacter->BehaviorTreeAsset);
	}
}

void ADEPRECATED_AAIArenaMinionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const auto AimActor = GetFocusOnActor();

	SetFocus(AimActor);

}

AActor* ADEPRECATED_AAIArenaMinionController::GetFocusOnActor() const
{
	if (!GetBlackboardComponent()) return nullptr;

	return Cast<AActor>(GetBlackboardComponent()->GetValueAsObject(FocusOnKeyName));
}
