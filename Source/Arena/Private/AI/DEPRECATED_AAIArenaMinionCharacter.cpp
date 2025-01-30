#include "AI/DEPRECATED_AAIArenaMinionCharacter.h"
#include "AI/DEPRECATED_AAIArenaMinionController.h"
#include "BrainComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Kismet/GameplayStatics.h>
#include <Character/DEPRECATED_ABlasterCharacter.h>
#include "Character/ArenaHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"

ADEPRECATED_AAIArenaMinionCharacter::ADEPRECATED_AAIArenaMinionCharacter()
{
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ADEPRECATED_AAIArenaMinionController::StaticClass();
	bUseControllerRotationYaw = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 400.0f, 0.0f);
	}

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);
}

void ADEPRECATED_AAIArenaMinionCharacter::OnDeathStarted(AActor* OwningActor)
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	check(MovementComp);
	MovementComp->StopMovementImmediately();
	MovementComp->DisableMovement();
}

void ADEPRECATED_AAIArenaMinionCharacter::OnDeathFinished(AActor* OwningActor)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	SetActorHiddenInGame(true);
}

void ADEPRECATED_AAIArenaMinionCharacter::SetTowerTarget(AActor* Target)
{
	TowerTarget = Target;
}

void ADEPRECATED_AAIArenaMinionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
