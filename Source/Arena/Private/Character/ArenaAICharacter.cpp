// Copyright Ludens Studio. All Rights Reserved.


#include "Character/ArenaAICharacter.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AArenaAICharacter::AArenaAICharacter()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("AttributeSet"));
	ArenaHealthSet->OnOutOfHealth.AddUObject(this, &AArenaAICharacter::HandleOutOfHealth);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	SetReplicates(true);
}

UArenaAbilitySystemComponent* AArenaAICharacter::GetArenaAbilitySystemComponent() const
{
	return Cast<UArenaAbilitySystemComponent>(AbilitySystemComponent);
}

UAbilitySystemComponent* AArenaAICharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UArenaHealthSet* AArenaAICharacter::GetArenaHealthSet() const
{
	return ArenaHealthSet;
}

void AArenaAICharacter::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	K2_OnOutOfHealth();

	Destroy();
}

// Called when the game starts or when spawned
void AArenaAICharacter::BeginPlay()
{
	Super::BeginPlay();
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	
	ArenaHealthSet->SetMaxHealth(50.0f);
	ArenaHealthSet->SetHealth(50.0f);
}
