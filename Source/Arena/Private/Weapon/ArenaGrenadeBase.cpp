// Copyright Ludens Studio. All Rights Reserved.

#include "Weapon/ArenaGrenadeBase.h"

#include "AbilitySystemComponent.h"
#include "ArenaGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayCueFunctionLibrary.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"
#include "GameplayEffect.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/App.h"
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
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

void AArenaGrenadeBase::SetGrenadeExplosionTimer_Implementation(float TimeBeforeExplosion)
{
	GetWorldTimerManager().ClearTimer(ExplosionCountdownTimerHandle);
	if (bDetonationFired)
	{
		return;
	}

	if (TimeBeforeExplosion > 0.0f)
	{
		GetWorldTimerManager().SetTimer(ExplosionCountdownTimerHandle, this, &AArenaGrenadeBase::Detonate, TimeBeforeExplosion, false);
	}
	else
	{
		// The throwing ability subtracts cooking and montage time. An expired fuse must not disarm the grenade.
		Detonate();
	}
}

void AArenaGrenadeBase::SetSourceAbilitySystem(UAbilitySystemComponent* InSourceAbilitySystem)
{
	if (InSourceAbilitySystem)
	{
		SourceAbilitySystem = InSourceAbilitySystem;
	}
}

UAbilitySystemComponent* AArenaGrenadeBase::GetSourceAbilitySystem() const
{
	if (SourceAbilitySystem.IsValid())
	{
		return SourceAbilitySystem.Get();
	}

	// A submunition spawned from another grenade inherits the original thrower.
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());
}

void AArenaGrenadeBase::SetSourceGrenadeDefinition(const UArenaGrenadeDefinitionData* InSourceGrenadeDefinition)
{
	if (InSourceGrenadeDefinition)
	{
		SourceGrenadeDefinition = InSourceGrenadeDefinition;
	}
}

const UArenaGrenadeDefinitionData* AArenaGrenadeBase::GetSourceGrenadeDefinition() const
{
	return SourceGrenadeDefinition ? SourceGrenadeDefinition.Get() : GrenadeDefinitionData.Get();
}

void AArenaGrenadeBase::BeginPlay()
{
	Super::BeginPlay();

	// The fuse can outlive the thrower, so capture the source while the Pawn is still alive.
	SourceAbilitySystem = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());

	if (ACharacter* Character = Cast<ACharacter>(GetInstigator()))
	{
		Character->GetCapsuleComponent()->IgnoreActorWhenMoving(this, true);
		CollisionComponent->IgnoreActorWhenMoving(Character, true);
	}
}

void AArenaGrenadeBase::Detonate_Implementation()
{
	if (bDetonationFired || GrenadeDefinitionData == nullptr)
	{
		return;
	}

	bDetonationFired = true;
	GetWorldTimerManager().ClearTimer(ExplosionCountdownTimerHandle);

	// VFX
	FGameplayCueParameters GameplayCueParameters;
	GameplayCueParameters.Location = GetActorLocation();
	if (GrenadeDefinitionData->ExplosionCueTag.IsValid() && GetNetMode() != NM_DedicatedServer && FApp::CanEverRender())
	{
		// The grenade is destroyed below; its burst cannot wait for a missing cue class to load asynchronously.
		PrepareExplosionCue();
		UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(this, GrenadeDefinitionData->ExplosionCueTag, GameplayCueParameters);
	}

	// Hide Mesh
	if (SpawnedCosmeticActor.Get())
	{
		SpawnedCosmeticActor->SetActorHiddenInGame(true);
	}

	// Deactivate Projectile Movement
	ProjectileMovementComponent->Deactivate();

	// On Server, capture all pawns and destructible objects within the explosion radius
	if (HasAuthority())
	{
		TArray<AActor*> OverlappingActors;
		if (GrenadeDefinitionData->ExplosionGameplayEffect && GetActorsWithinExplosionRadius(OverlappingActors))
		{
			for (AActor* OverlappingActor : OverlappingActors)
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
				bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), OverlappingActor->GetActorLocation(), UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1), true, IgnoreActors, DrawDebugType, HitResult, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

				if (bHit && HitResult.GetActor() == OverlappingActor)
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

		SpawnActorsOnDetonation();
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

	if (GrenadeDefinitionData == nullptr)
	{
		return bShouldDetonate;
	}

	if (GrenadeDefinitionData->ImpactDetonationPolicy.HasTagExact(TAG_DetonationPolicy_OnImpact))
	{
		bShouldDetonate = true;
	}

	if (GrenadeDefinitionData->ImpactDetonationPolicy.HasTagExact(TAG_DetonationPolicy_OnHitValidTarget))
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitResult.GetActor()))
		{
			bShouldDetonate |= IsValid(TargetASC);
		}
	}

	if (GrenadeDefinitionData->ImpactDetonationPolicy.HasTagExact(TAG_DetonationPolicy_OnHitHorizontal))
	{
		bShouldDetonate |= HitResult.ImpactNormal.Z > 0.7f;
	}

	return bShouldDetonate;
}

void AArenaGrenadeBase::SpawnEffectActor_Implementation(const FTransform& SpawnTransform,
	const TSubclassOf<AArenaEffectActor> EffectActorClass, const UArenaEffectActorDefinition* EffectDefinition,
	int32 SpawnCount, float SpawnRadius)
{
	if (EffectActorClass == nullptr || SpawnCount <= 0 || !HasAuthority())
	{
		return;
	}

	// Golden angle in radians: PI * (3 - sqrt(5))
	constexpr float GoldenAngleRad = 2.39996323f;
	const int32 Count = FMath::Min(SpawnCount, 64);
	const float MaxRadius = FMath::Max(0.f, SpawnRadius);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Sunflower disk sampling is deterministic; the first patch remains at the spawn origin.
		const float Radius = Count > 1 ? MaxRadius * FMath::Sqrt(static_cast<float>(Index) / (Count - 1)) : 0.f;
		const float Angle = Index * GoldenAngleRad;
		FTransform NewSpawnTransform = SpawnTransform;
		NewSpawnTransform.AddToTranslation(FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f));

		AArenaEffectActor* SpawnedEffectActor = GetWorld()->SpawnActorDeferred<AArenaEffectActor>(
			EffectActorClass, NewSpawnTransform, GetOwner(), GetInstigator(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!SpawnedEffectActor)
		{
			continue;
		}

		// Ignore collision between the spawned effect actor and the grenade
		CollisionComponent->IgnoreActorWhenMoving(SpawnedEffectActor, true);
		// Hand over the source that was captured at launch; the thrower's Pawn may already be gone.
		SpawnedEffectActor->SetSource(GetInstigator(), GetSourceAbilitySystem());
		SpawnedEffectActor->SetSourceGrenadeDefinition(GetSourceGrenadeDefinition());
		SpawnedEffectActor->InitializeEffect(EffectDefinition);
		SpawnedEffectActor->FinishSpawning(NewSpawnTransform, true);
	}
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
	for (int32 Index = 0; Index < SpawnCount; Index++)
	{
		const FRotator Rotation = FRotator(30.0f, Index * AngleIncrement, 0.0f);
		FVector ForwardVector = UKismetMathLibrary::GetForwardVector(Rotation);
		FVector SpawnLocation = SpawnTransform.GetLocation() + (ForwardVector * 50.0f);
		FTransform NewSpawnTransform = FTransform(Rotation, SpawnLocation);
		if (AArenaGrenadeBase* Spawned = UArenaSystemStatics::SpawnGrenadeByGrenadeDefinition(
			this, NewSpawnTransform, GrenadeDefinition, GetOwner(), GetInstigator()))
		{
			// Inherit the original thrower so a submunition chain keeps attribution after the Pawn dies.
			Spawned->SetSourceAbilitySystem(GetSourceAbilitySystem());
			Spawned->SetSourceGrenadeDefinition(GetSourceGrenadeDefinition());
		}
	}
}

void AArenaGrenadeBase::LaunchGrenade()
{
	PrepareExplosionCue();
	SetupVFX();

	if (GrenadeDefinitionData->TimeBeforeExplosion > 0.0f)
	{
		GetWorldTimerManager().SetTimer(ExplosionCountdownTimerHandle, this, &AArenaGrenadeBase::Detonate, GrenadeDefinitionData->TimeBeforeExplosion, false);
	}

	FTimerHandle PostLaunchCleanupTimerHandle;
	GetWorldTimerManager().SetTimer(PostLaunchCleanupTimerHandle, this, &AArenaGrenadeBase::PostLaunchCleanup, 0.15f, false);
}

void AArenaGrenadeBase::PrepareExplosionCue()
{
	if (!GrenadeDefinitionData || !GrenadeDefinitionData->ExplosionCueTag.IsValid() ||
		GetNetMode() == NM_DedicatedServer || !FApp::CanEverRender())
	{
		return;
	}

	UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
	UGameplayCueSet* CueSet = CueManager ? CueManager->GetRuntimeCueSet() : nullptr;
	const int32* CueIndex = CueSet ? CueSet->GameplayCueDataMap.Find(GrenadeDefinitionData->ExplosionCueTag) : nullptr;
	if (!CueIndex)
	{
		return;
	}

	int32 Index = *CueIndex;
	while (CueSet->GameplayCueData.IsValidIndex(Index))
	{
		FGameplayCueNotifyData& CueData = CueSet->GameplayCueData[Index];
		if (!CueData.LoadedGameplayCueClass)
		{
			CueData.LoadedGameplayCueClass = Cast<UClass>(CueData.GameplayCueNotifyObj.TryLoad());
		}

		Index = CueData.ParentDataIdx;
	}
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

void AArenaGrenadeBase::ApplyDamageToTarget(const AActor* Target, const FHitResult& HitResult, bool bIsDirectHit) const
{
	if (!HasAuthority())
	{
		return;
	}

	// Use the source captured at launch: the thrower may already be dead by the time the fuse expires.
	UAbilitySystemComponent* InstigatorASC = GetSourceAbilitySystem();
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

	if (!TargetASC || !InstigatorASC)
	{
		return;
	}

	FGameplayEffectContextHandle GameplayEffectContextHandle = InstigatorASC->MakeEffectContext();
	GameplayEffectContextHandle.AddHitResult(HitResult, true);
	// The definition asset outlives this Actor, so eliminations can still credit the grenade after it is destroyed.
	GameplayEffectContextHandle.AddSourceObject(GetSourceGrenadeDefinition());

	const float DistanceToCenter = HitResult.Distance;
	const float EffectLevel = FMath::Clamp(DistanceToCenter / GrenadeDefinitionData->DetonationRadius, 0.1f, 1.0f);
	if (const TSubclassOf<UGameplayEffect> ExplosionGameplayEffect = bIsDirectHit ? GrenadeDefinitionData->DirectHitGameplayEffect : GrenadeDefinitionData->ExplosionGameplayEffect)
	{
		FGameplayEffectSpecHandle DamageSpec = InstigatorASC->MakeOutgoingSpec(ExplosionGameplayEffect, EffectLevel, GameplayEffectContextHandle);
		if (DamageSpec.IsValid())
		{
			if (bIsDirectHit)
			{
				DamageSpec.Data->AddDynamicAssetTag(ArenaGameplayTags::DamageType_Grenade_DirectHit);
			}
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		}
	}
}

void AArenaGrenadeBase::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!GrenadeDefinitionData || bDetonationFired)
	{
		return;
	}

	if (ShouldDetonateOnImpact(Hit))
	{
		UKismetSystemLibrary::K2_ClearTimerHandle(this, ExplosionCountdownTimerHandle);
		DirectHitTarget = Other;
		ApplyDamageToTarget(Other, Hit, true);
		Detonate();
	}
	else
	{
		SpawnActorsOnHit(Hit);
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

	SpawnedCosmeticActor = GetWorld()->SpawnActorDeferred<AActor>(GrenadeDefinitionData->CosmeticActorClass, GetActorTransform());
	SpawnedCosmeticActor->FinishSpawning(FTransform::Identity, true);
	SpawnedCosmeticActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	SpawnedCosmeticActor->SetActorEnableCollision(false);

	FVector NewRelativeLocation = SpawnedCosmeticActor->GetRootComponent()->GetRelativeLocation();
	NewRelativeLocation.Z -= 10.0f;
	SpawnedCosmeticActor->SetActorRelativeLocation(NewRelativeLocation);
}

void AArenaGrenadeBase::SpawnActorsOnHit(const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	ProcessEffectActorEntries(FTransform(Hit.ImpactNormal.Rotation(), Hit.ImpactPoint), ESpawnPolicy::SpawnOnHit);
	ProcessSecondaryGrenadeEntries(FTransform(Hit.ImpactNormal.Rotation(), Hit.ImpactPoint), ESpawnPolicy::SpawnOnHit);
}

void AArenaGrenadeBase::SpawnActorsOnDetonation()
{
	if (!HasAuthority())
	{
		return;
	}

	ProcessEffectActorEntries(GetActorTransform(), ESpawnPolicy::SpawnOnDetonation);
	ProcessSecondaryGrenadeEntries(FTransform(GetActorRotation(), GetActorLocation()), ESpawnPolicy::SpawnOnDetonation);
}

void AArenaGrenadeBase::ProcessEffectActorEntries(const FTransform& SpawnTransform, ESpawnPolicy Policy)
{
	if (!HasAuthority())
	{
		return;
	}

	for (const FEffectActorSpawnData& Spawn : GrenadeDefinitionData->EffectActorsToSpawn)
	{
		if (Spawn.SpawnPolicy != Policy || !Spawn.EffectActorClass)
		{
			continue;
		}

		SpawnEffectActor(SpawnTransform, Spawn.EffectActorClass, Spawn.EffectDefinition, Spawn.SpawnCount, Spawn.SpawnRadius);
	}
}

void AArenaGrenadeBase::ProcessSecondaryGrenadeEntries(const FTransform& SpawnTransform, ESpawnPolicy Policy)
{
	if (!HasAuthority())
	{
		return;
	}

	for (const FSecondaryGrenadeSpawnData& Spawn : GrenadeDefinitionData->SecondaryGrenadesToSpawn)
	{
		if (Spawn.SpawnPolicy != Policy || !Spawn.GrenadeDefinitionData)
		{
			continue;
		}

		SpawnSecondaryGrenade(SpawnTransform, Spawn.GrenadeDefinitionData, Spawn.SpawnCount);
	}
}
