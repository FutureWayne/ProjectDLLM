// Copyright Ludens Studio. All Rights Reserved.


#include "Weapon/ArenaGrenadeBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayCueFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Actor/ArenaEffectActor.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "System/ArenaSystemStatics.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

// Sets default values
AArenaGrenadeBase::AArenaGrenadeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	CollisionComponent->SetCollisionProfileName("BlockAllDynamic");
	CollisionComponent->SetCollisionObjectType(ECC_GameTraceChannel1);
	CollisionComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	SetRootComponent(CollisionComponent);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bAutoActivate = true;
	ProjectileMovementComponent->bInterpMovement = true;
	ProjectileMovementComponent->bInterpRotation = true;
	
	SetReplicates(true);
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(100.0f);
}

void AArenaGrenadeBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AArenaGrenadeBase, GrenadeDefinitionData);
}

void AArenaGrenadeBase::SetGrenadeParameter_Implementation(const UArenaGrenadeDefinitionData* InGrenadeDefinitionData)
{
	if (InGrenadeDefinitionData)
	{
		GrenadeDefinitionData = InGrenadeDefinitionData;
		ProjectileMovementComponent->InitialSpeed = GrenadeDefinitionData->ProjectileSpeed;
		ProjectileMovementComponent->MaxSpeed = GrenadeDefinitionData->ProjectileSpeed;
		ProjectileMovementComponent->bShouldBounce = GrenadeDefinitionData->bShouldBounce;
		ProjectileMovementComponent->ProjectileGravityScale = GrenadeDefinitionData->GravityScale;
		ProjectileMovementComponent->Bounciness = GrenadeDefinitionData->Bounciness;
		ProjectileMovementComponent->Friction = GrenadeDefinitionData->Friction;
		ProjectileMovementComponent->MinFrictionFraction = GrenadeDefinitionData->MinFrictionFraction;
		ProjectileMovementComponent->Velocity = GetActorForwardVector() * GrenadeDefinitionData->ProjectileSpeed;

		SpawnCosmeticActor();
		LaunchGrenade();
	}
}

void AArenaGrenadeBase::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* Character = Cast<ACharacter>(GetInstigator()))
	{
		Character->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
		CollisionComponent->IgnoreActorWhenMoving(Character, true);
	}
}


void AArenaGrenadeBase::Detonate_Implementation()
{
	if (bDetonationFired)
	{
		return;
	}

	bDetonationFired = true;
	
	// VFX
	FGameplayCueParameters GameplayCueParameters;
	GameplayCueParameters.Location = GetActorLocation();
	UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(this, GrenadeDefinitionData->ExplosionCueTag, GameplayCueParameters);

	// Hide Mesh
	if (SpawnedCosmeticActor.Get())
	{
		SpawnedCosmeticActor->SetActorHiddenInGame(true);
	}
	
	// Deactivate Projectile Movement
	ProjectileMovementComponent->Deactivate();

	if (HasAuthority())
	{
		TArray<AActor*> OverlappingActors;
		if (GetActorsWithinExplosionRadius(OverlappingActors))
		{
			for (auto OverlappingActor : OverlappingActors)
			{
				TArray<AActor*> IgnoreActors = OverlappingActors;
				
				if (DirectHitTarget.IsValid())
				{
					IgnoreActors.Add(DirectHitTarget.Get());
				}
				
				if (!IsValid(OverlappingActor))
				{
					continue;
				}
				
				IgnoreActors.Remove(OverlappingActor);
				// Trace to check for valid line of sight while ignoring other pawns in radius, so they don't block the hit
				FHitResult HitResult;
				EDrawDebugTrace::Type DrawDebugType = bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;
				bool hit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), OverlappingActor->GetActorLocation(), UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1), true, IgnoreActors, DrawDebugType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

				if (hit)
				{
					if (bDrawDebug)
					{
						DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.0f, 12, FColor::Green, false, 5.0f);
					}
					ApplyDamageToTarget(OverlappingActor, HitResult, false);
				}
			}
		}

		if (IsValid(SpawnedCosmeticActor))
		{
			SpawnedCosmeticActor->Destroy();
		}

		CheckSpawnConditionOnDetonation();
		PostDetonation();
		Destroy();
	}
}


void AArenaGrenadeBase::PostDetonation_Implementation()
{
}

bool AArenaGrenadeBase::ShouldDetonateOnImpact_Implementation(FHitResult HitResult) const
{
	bool bShouldDetonate = false;

	if (GrenadeDefinitionData->DetonationPolicy.HasTagExact(TAG_DetonationPolicy_OnImpact))
	{
		bShouldDetonate = true;
	}

	if (GrenadeDefinitionData->DetonationPolicy.HasTagExact(TAG_DetonationPolicy_OnHitValidTarget))
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitResult.GetActor()))
		{
			bShouldDetonate |= IsValid(TargetASC);
		}
	}

	if (GrenadeDefinitionData->DetonationPolicy.HasTagExact(TAG_DetonationPolicy_OnHitHorizontal))
	{
		bShouldDetonate |= HitResult.ImpactNormal.Z > 0.7f;
	}

	return bShouldDetonate;
}

void AArenaGrenadeBase::SpawnEffectActor_Implementation(const FTransform& SpawnTransform,
                                                        const TSubclassOf<AArenaEffectActor> EffectActorClass)
{
	if (EffectActorClass == nullptr || !HasAuthority())
	{
		return;
	}

	AArenaEffectActor* SpawnedEffectActor = GetWorld()->SpawnActorDeferred<AArenaEffectActor>(EffectActorClass, SpawnTransform);
	// Ignore collision between the spawned effect actor and the grenade
	CollisionComponent->IgnoreActorWhenMoving(SpawnedEffectActor, true);
	SpawnedEffectActor->FinishSpawning(SpawnTransform, true);
}

void AArenaGrenadeBase::SpawnSecondaryGrenade_Implementation(const FTransform& SpawnTransform,
	const UArenaGrenadeDefinitionData* GrenadeDefinition, int32 SpawnCount)
{
	if (GrenadeDefinition == nullptr || SpawnCount <= 0 || !HasAuthority())
	{
		return;
	}

	// Evenly distribute the spawn count around the spawn transform
	const float AngleIncrement = 360.0f / SpawnCount;
	for (int32 i = 0; i < SpawnCount; i++)
	{
		const FRotator Rotation = FRotator(30.0f, i * AngleIncrement, 0.0f);
		FVector ForwardVector = UKismetMathLibrary::GetForwardVector(Rotation);
		FVector SpawnLocation = SpawnTransform.GetLocation() + (ForwardVector * 50.0f);
		FTransform NewSpawnTransform = FTransform(Rotation, SpawnLocation);
		UArenaSystemStatics::SpawnGrenadeByGrenadeDefinition(this, NewSpawnTransform, GrenadeDefinition, GetOwner(), Cast<APawn>(GetInstigator()));
	}
}

void AArenaGrenadeBase::LaunchGrenade()
{
	SetupVFX();

	if (GrenadeDefinitionData->TimeBeforeExplosion > 0.0f)
	{
		GetWorldTimerManager().SetTimer(ExplosionCountdownTimerHandle, this, &AArenaGrenadeBase::Detonate, GrenadeDefinitionData->TimeBeforeExplosion, false);
	}

	FTimerHandle PostLaunchCleanupTimerHandle;
	GetWorldTimerManager().SetTimer(PostLaunchCleanupTimerHandle, this, &AArenaGrenadeBase::PostLaunchCleanup, 0.15f, false);
}

void AArenaGrenadeBase::SetupVFX()
{
	// Setup Trail Effect
	if (GrenadeDefinitionData->TrailEffect)
	{
		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(GrenadeDefinitionData->TrailEffect, CollisionComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, true);
	}

	if (UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>())
	{
		TeamColor = TeamSubsystem->FindTeamColorFromActor(GetInstigator());

		if (TrailComponent)
		{
			TrailComponent->SetVariableLinearColor(FName("Team_Color"), TeamColor);
		}
	}
}

void AArenaGrenadeBase::PostLaunchCleanup()
{
	// Reenable pawn / grenade collision, allow velocity to drive rotation
	if (ACharacter* Character = Cast<ACharacter>(GetInstigator()))
	{
		Character->GetCapsuleComponent()->IgnoreActorWhenMoving(this, false);
		CollisionComponent->IgnoreActorWhenMoving(Character, false);
	}
}

bool AArenaGrenadeBase::GetActorsWithinExplosionRadius(TArray<AActor*>& OutOverlappingActors) const
{
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), GrenadeDefinitionData->DetonationRadius, 12, FColor::Red, false, 5.0f);
	}
	
	// Capture all pawns in detonation radius
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Destructible));
	return UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), GrenadeDefinitionData->DetonationRadius, ObjectTypes, nullptr, TArray<AActor*>(), OutOverlappingActors);
}

void AArenaGrenadeBase::ApplyDamageToTarget(const AActor* Target, const FHitResult& HitResult, bool IsDirectHit) const
{
	if (!HasAuthority())
	{
		return;
	}
	
	UAbilitySystemComponent* InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

	if (!TargetASC || !InstigatorASC)
	{
		return;
	}

	FGameplayEffectContextHandle GameplayEffectContextHandle = InstigatorASC->MakeEffectContext();
	GameplayEffectContextHandle.AddHitResult(HitResult, true);

	const float DistanceToCenter = HitResult.Distance;
	const float EffectLevel = FMath::Clamp(DistanceToCenter / GrenadeDefinitionData->DetonationRadius, 0.1f, 1.0f);
	const TSubclassOf<UGameplayEffect> ExplosionGameplayEffect = IsDirectHit ? GrenadeDefinitionData->DirectHitGameplayEffect : GrenadeDefinitionData->ExplosionGameplayEffect;
	check(ExplosionGameplayEffect);
	UGameplayEffect* ExplosionGameplayEffectCDO = ExplosionGameplayEffect->GetDefaultObject<UGameplayEffect>();
	InstigatorASC->ApplyGameplayEffectToTarget(ExplosionGameplayEffectCDO, TargetASC, EffectLevel, GameplayEffectContextHandle);
}

void AArenaGrenadeBase::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShouldDetonateOnImpact(Hit))
	{
		UKismetSystemLibrary::K2_ClearTimerHandle(this, ExplosionCountdownTimerHandle);
		DirectHitTarget = Other;
		ApplyDamageToTarget(Other, Hit, true);
		Detonate();
	}
	else
	{
		CheckSpawnConditionOnHit(Hit);
		UGameplayStatics::SpawnSoundAtLocation(this, GrenadeDefinitionData->GrenadeImpactSound, Hit.Location);
	}

	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
}

void AArenaGrenadeBase::SpawnCosmeticActor()
{
	if (GrenadeDefinitionData->CosmeticActorClass == nullptr || !HasAuthority())
	{
		return;
	}
	
	SpawnedCosmeticActor = GetWorld()->SpawnActorDeferred<AActor>( GrenadeDefinitionData->CosmeticActorClass, GetActorTransform());
	SpawnedCosmeticActor->FinishSpawning(FTransform::Identity, true);
	SpawnedCosmeticActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	SpawnedCosmeticActor->SetActorEnableCollision(false);
	
	FVector NewRelativeLocation = SpawnedCosmeticActor->GetRootComponent()->GetRelativeLocation();
	NewRelativeLocation.Z -= 10.0f;
	SpawnedCosmeticActor->SetActorRelativeLocation(NewRelativeLocation);
}

void AArenaGrenadeBase::CheckSpawnConditionOnHit(const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}
	
	for (const auto EffectActorSpawnData : GrenadeDefinitionData->EffectActorsToSpawn)
	{
		if (EffectActorSpawnData.SpawnPolicy == ESpawnPolicy::DoNotSpawn)
		{
			continue;
		}

		if (EffectActorSpawnData.EffectActorClass == nullptr)
		{
			continue;
		}

		if (EffectActorSpawnData.SpawnPolicy == ESpawnPolicy::SpawnOnHit)
		{
			FTransform SpawnTransform = FTransform(Hit.ImpactNormal.Rotation(), Hit.ImpactPoint);
			SpawnEffectActor(SpawnTransform, EffectActorSpawnData.EffectActorClass);
		}
	}

	for (const auto SecondaryGrenadeSpawnData : GrenadeDefinitionData->SecondaryGrenadesToSpawn)
	{
		if (SecondaryGrenadeSpawnData.SpawnPolicy == ESpawnPolicy::DoNotSpawn)
		{
			continue;
		}

		if (SecondaryGrenadeSpawnData.GrenadeDefinitionData == nullptr)
		{
			continue;
		}

		if (SecondaryGrenadeSpawnData.SpawnPolicy == ESpawnPolicy::SpawnOnHit)
		{
			FTransform SpawnTransform = FTransform(Hit.ImpactNormal.Rotation(), Hit.ImpactPoint);
			SpawnSecondaryGrenade(SpawnTransform, SecondaryGrenadeSpawnData.GrenadeDefinitionData, SecondaryGrenadeSpawnData.SpawnCount);
		}
	}
}

void AArenaGrenadeBase::CheckSpawnConditionOnDetonation()
{
	if (!HasAuthority() )
	{
		return;
	}
	
	for (const auto EffectActorSpawnData : GrenadeDefinitionData->EffectActorsToSpawn)
	{
		if (EffectActorSpawnData.SpawnPolicy == ESpawnPolicy::DoNotSpawn)
		{
			continue;
		}

		if (EffectActorSpawnData.EffectActorClass == nullptr)
		{
			continue;
		}

		if (EffectActorSpawnData.SpawnPolicy == ESpawnPolicy::SpawnOnDetonation)
		{
			FTransform SpawnTransform = FTransform(GetActorRotation(), GetActorLocation());
			SpawnEffectActor(SpawnTransform, EffectActorSpawnData.EffectActorClass);
		}
	}

	for (const auto SecondaryGrenadeSpawnData : GrenadeDefinitionData->SecondaryGrenadesToSpawn)
	{
		if (SecondaryGrenadeSpawnData.SpawnPolicy == ESpawnPolicy::DoNotSpawn)
		{
			continue;
		}

		if (SecondaryGrenadeSpawnData.GrenadeDefinitionData == nullptr)
		{
			continue;
		}

		if (SecondaryGrenadeSpawnData.SpawnPolicy == ESpawnPolicy::SpawnOnDetonation)
		{
			FTransform SpawnTransform = FTransform(GetActorRotation(), GetActorLocation());
			SpawnSecondaryGrenade(SpawnTransform, SecondaryGrenadeSpawnData.GrenadeDefinitionData, SecondaryGrenadeSpawnData.SpawnCount);
		}
	}
}
