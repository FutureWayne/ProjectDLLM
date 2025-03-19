// Copyright Ludens Studio. All Rights Reserved.


#include "Actor/ArenaDestructibleActor.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"

// Sets default values
AArenaDestructibleActor::AArenaDestructibleActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("AttributeSet"));
	ArenaHealthSet->OnOutOfHealth.AddUObject(this, &AArenaDestructibleActor::HandleOutOfHealth);
}

UArenaAbilitySystemComponent* AArenaDestructibleActor::GetArenaAbilitySystemComponent() const
{
	return Cast<UArenaAbilitySystemComponent>(AbilitySystemComponent);
}

UAbilitySystemComponent* AArenaDestructibleActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UArenaHealthSet* AArenaDestructibleActor::GetArenaHealthSet() const
{
	return ArenaHealthSet;
}

// Called when the game starts or when spawned
void AArenaDestructibleActor::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void AArenaDestructibleActor::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	K2_OnOutOfHealth(DamageInstigator, DamageCauser);

	Destroy();
}


