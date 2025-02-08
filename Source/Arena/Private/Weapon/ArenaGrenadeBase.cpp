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
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

// Sets default values
AArenaGrenadeBase::AArenaGrenadeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	CollisionComponent->SetCollisionProfileName("BlockAllDynamic");
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

void AArenaGrenadeBase::SetGrenadeParameter_Implementation(UArenaGrenadeDefinitionData* InGrenadeDefinitionData)
{
	if (InGrenadeDefinitionData)
	{
		GrenadeDefinitionData = InGrenadeDefinitionData;
		ProjectileMovementComponent->InitialSpeed = GrenadeDefinitionData->ProjectileSpeed;
		ProjectileMovementComponent->MaxSpeed = GrenadeDefinitionData->ProjectileSpeed;
		ProjectileMovementComponent->bShouldBounce = GrenadeDefinitionData->bShouldBounce;
		ProjectileMovementComponent->ProjectileGravityScale = GrenadeDefinitionData->GravityScale;
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
				bool hit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), OverlappingActor->GetActorLocation(), UEngineTypes::ConvertToTraceType(ECC_Visibility), true, IgnoreActors, EDrawDebugTrace::ForDuration, HitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

				if (hit)
				{
					ApplyDamageToTarget(OverlappingActor, HitResult, false);
				}
			}
		}

		if (IsValid(SpawnedCosmeticActor))
		{
			SpawnedCosmeticActor->Destroy();
		}
		
		PostDetonation();
		Destroy();
	}
}

bool AArenaGrenadeBase::ShouldDetonateOnImpact_Implementation(FHitResult HitResult) const
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitResult.GetActor());
	return IsValid(TargetASC);
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
	NewRelativeLocation.Z -= 5.0f;
	SpawnedCosmeticActor->SetActorRelativeLocation(NewRelativeLocation);
}