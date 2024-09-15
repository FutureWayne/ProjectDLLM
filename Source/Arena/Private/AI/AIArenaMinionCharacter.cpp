#include "AI/AIArenaMinionCharacter.h"
#include "AI/AIArenaMinionController.h"
#include "BrainComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Kismet/GameplayStatics.h>
#include <Character/BlasterCharacter.h>

AAIArenaMinionCharacter::AAIArenaMinionCharacter()
{
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIArenaMinionController::StaticClass();
	bUseControllerRotationYaw = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 400.0f, 0.0f);
	}
}

void AAIArenaMinionCharacter::SetTowerTarget(AActor* Target)
{
	TowerTarget = Target;
}

void AAIArenaMinionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
