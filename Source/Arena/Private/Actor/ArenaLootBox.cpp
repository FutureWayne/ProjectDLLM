// Copyright Ludens Studio. All Rights Reserved.


#include "Actor/ArenaLootBox.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Actor/ArenaLootBoxData.h"
#include "Actor/ArenaWeaponSpawner.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Equipment/ArenaPickupDefinition.h"

// Sets default values
AArenaLootBox::AArenaLootBox()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	ArenaHealthSet = CreateDefaultSubobject<UArenaHealthSet>(TEXT("AttributeSet"));
	ArenaHealthSet->OnOutOfHealth.AddUObject(this, &AArenaLootBox::HandleOutOfHealth);

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxComponent->SetCollisionResponseToAllChannels(ECR_Block);
	SetRootComponent(BoxComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(BoxComponent);

	SetReplicates(true);
}

UArenaAbilitySystemComponent* AArenaLootBox::GetArenaAbilitySystemComponent() const
{
	return Cast<UArenaAbilitySystemComponent>(AbilitySystemComponent);
}

UAbilitySystemComponent* AArenaLootBox::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UArenaHealthSet* AArenaLootBox::GetArenaHealthSet() const
{
	return ArenaHealthSet;
}

// Called when the game starts or when spawned
void AArenaLootBox::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void AArenaLootBox::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser,
	const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	UArenaWeaponPickupDefinition* PickupDefinition = LootBoxData->GetRandomPickupDefinition();
	if (PickupDefinition && WeaponSpawnerClass)
	{
		AArenaWeaponSpawner* WeaponSpawner = GetWorld()->SpawnActorDeferred<AArenaWeaponSpawner>(WeaponSpawnerClass, GetActorTransform(), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (WeaponSpawner)
		{
			WeaponSpawner->SetPickupDefinition(PickupDefinition);
			WeaponSpawner->OnConstruction(GetActorTransform());
		}

		WeaponSpawner->FinishSpawning(GetActorTransform());
	}

	Destroy();
}
