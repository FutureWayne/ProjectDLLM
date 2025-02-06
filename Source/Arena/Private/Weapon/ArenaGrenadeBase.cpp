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
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Teams/ArenaTeamSubsystem.h"

// Sets default values
AArenaGrenadeBase::AArenaGrenadeBase()
{
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = ProjectileSpeed;
	ProjectileMovementComponent->MaxSpeed = ProjectileSpeed;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = bShouldBounce;
	
	SetReplicates(true);
}

void AArenaGrenadeBase::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* Character = Cast<ACharacter>(GetInstigator()))
	{
		Character->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
		MeshComponent->IgnoreActorWhenMoving(Character, true);
	}

	SetupVFX();

	if (TimeBeforeExplosion > 0.0f)
	{
		GetWorldTimerManager().SetTimer(ExplosionCountdownTimerHandle, this, &AArenaGrenadeBase::Detonate, TimeBeforeExplosion, false);
	}

	FTimerHandle PostLaunchCleanupTimerHandle;
	GetWorldTimerManager().SetTimer(PostLaunchCleanupTimerHandle, this, &AArenaGrenadeBase::PostLaunchCleanup, 0.15f, false);
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
	UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(this, ExplosionCueTag, GameplayCueParameters);

	// Hide Mesh
	MeshComponent->SetHiddenInGame(true);

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
					ApplyDamageToTarget(OverlappingActor, HitResult);
				}
			}
		}

		if (TrailComponent)
		{
			TrailComponent->OnSystemFinished.AddDynamic(this, &AArenaGrenadeBase::OnTrailFinished);
		}
		else
		{
			Destroy();
		}
	}
}

bool AArenaGrenadeBase::ShouldDetonate_Implementation(FHitResult HitResult) const
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitResult.GetActor());
	return IsValid(TargetASC);
}

void AArenaGrenadeBase::SetupVFX()
{
	// Setup Trail Effect
	if (TrailEffect)
	{
		TrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailEffect, MeshComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
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
		MeshComponent->IgnoreActorWhenMoving(Character, false);
	}

	ProjectileMovementComponent->bRotationFollowsVelocity = true;
}

bool AArenaGrenadeBase::GetActorsWithinExplosionRadius(TArray<AActor*>& OutOverlappingActors) const
{
	// Capture all pawns in detonation radius
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Destructible));
	return UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), DetonationRadius, ObjectTypes, nullptr, TArray<AActor*>(), OutOverlappingActors);
}

void AArenaGrenadeBase::OnTrailFinished(UNiagaraComponent* PSystem)
{
	Destroy();
}

void AArenaGrenadeBase::ApplyDamageToTarget(const AActor* Target, const FHitResult& HitResult) const
{
	UAbilitySystemComponent* InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

	if (!TargetASC)
	{
		return;
	}
	
	FGameplayEffectContextHandle GameplayEffectContextHandle;
	
	if (ensureMsgf(InstigatorASC, TEXT("Instigator of Grenade should ")))
	{
		GameplayEffectContextHandle = InstigatorASC->MakeEffectContext();
		GameplayEffectContextHandle.AddHitResult(HitResult, true);
	}

	float DistanceToCenter = HitResult.Distance;
	float EffectLevel = FMath::Clamp(DistanceToCenter / DetonationRadius, 0.1f, 1.0f);

	UGameplayEffect* ExplosionGameplayEffectCDO = ExplosionGameplayEffect->GetDefaultObject<UGameplayEffect>();
	InstigatorASC->ApplyGameplayEffectToTarget(ExplosionGameplayEffectCDO, TargetASC, EffectLevel, GameplayEffectContextHandle);
}

void AArenaGrenadeBase::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (ShouldDetonate(Hit))
	{
		UKismetSystemLibrary::K2_ClearTimerHandle(this, ExplosionCountdownTimerHandle);

		Detonate();
	}
	else
	{
		UGameplayStatics::SpawnSoundAtLocation(this, GrenadeImpactSound, Hit.Location);
	}
}