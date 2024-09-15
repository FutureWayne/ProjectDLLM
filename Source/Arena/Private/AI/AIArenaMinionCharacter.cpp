#include "AI/AIArenaMinionCharacter.h"
#include "AI/AIArenaMinionController.h"
#include "BrainComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	//bUseControllerRotationYaw = false;
}
