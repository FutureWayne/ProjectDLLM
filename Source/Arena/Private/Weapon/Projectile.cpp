// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Projectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Teams/ArenaTeamSubsystem.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			NAME_None,
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
		);
	}

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}

	// Ignore collision with the owner
	if (AActor* OwnerActor = GetOwner())
	{
		CollisionBox->IgnoreActorWhenMoving(OwnerActor, true);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		Destroy();
		return;
	}

	// Check whether hit actor is in the same team
	UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
	if (!ensure(TeamSubsystem))
	{
		Destroy();
		return;
	}
	
	if (!TeamSubsystem->CanCauseDamage(GetOwner(), OtherActor))
	{
		Destroy();
		return;
	}
	
	if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor))
	{
		BlasterCharacter->PlayHitReactMontage();
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor))
	{
		check(DamageEffectClass);
		FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffectClass, 1, EffectContext);
		const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
	Destroy();
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (ImpactEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, GetActorLocation(), GetActorRotation());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
	}
}

