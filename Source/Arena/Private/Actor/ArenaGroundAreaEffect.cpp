// Copyright Ludens Studio. All Rights Reserved.

#include "Actor/ArenaGroundAreaEffect.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/HitResult.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "TimerManager.h"
#include "Actor/ArenaGroundAreaTrace.h"

namespace
{
	UMaterialInstanceDynamic* BindAreaMaterial(UNiagaraComponent* Visual, UNiagaraSystem* Effect, FVector Center, float Radius)
	{
		const FNiagaraVariable Parameter(FNiagaraTypeDefinition::GetUMaterialDef(), TEXT("User.GroundAreaMaterial"));
		if (Effect->GetExposedParameters().IndexOf(Parameter) == INDEX_NONE)
		{
			return nullptr;
		}

		UMaterialInterface* Template = Cast<UMaterialInterface>(Effect->GetExposedParameters().GetUObject(Parameter));
		if (!Template)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Template, Visual);
		Material->SetVectorParameterValue(TEXT("GroundAreaCenter"), FLinearColor(Center.X, Center.Y, Center.Z));
		Material->SetScalarParameterValue(TEXT("GroundAreaRadius"), Radius);
		Material->SetScalarParameterValue(TEXT("GroundAreaMaskEnabled"), 1.f);
		Visual->SetVariableMaterial(Parameter.GetName(), Material);
		return Material;
	}
}

AArenaGroundAreaEffect::AArenaGroundAreaEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = false;
	SetReplicateMovement(false);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AArenaGroundAreaEffect::Initialize(const FArenaGroundAreaSettings& InSettings)
{
	check(!HasActorBegunPlay());
	Settings = InSettings;
	Settings.Sanitize();
}

void AArenaGroundAreaEffect::InitializeEffect_Implementation(const UArenaEffectActorDefinition* Definition)
{
	Super::InitializeEffect_Implementation(Definition);
	if (const UArenaGroundAreaDefinition* Area = Cast<UArenaGroundAreaDefinition>(Definition))
	{
		Initialize(Area->Settings);
	}
}

void AArenaGroundAreaEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AArenaGroundAreaEffect, Settings, COND_InitialOnly);
	DOREPLIFETIME(AArenaGroundAreaEffect, ActiveCells);
	DOREPLIFETIME(AArenaGroundAreaEffect, Placement);
}

void AArenaGroundAreaEffect::BeginPlay()
{
	Super::BeginPlay();
	Settings.Sanitize();
	if (!HasAuthority())
	{
		return;
	}

	SetActorRotation(FRotator(0., GetActorRotation().Yaw, 0.));
	SetActorScale3D(FVector::OneVector);
	PrepareGameplayEffect();
	if (const UArenaTeamSubsystem* Teams = GetWorld()->GetSubsystem<UArenaTeamSubsystem>())
	{
		SetGenericTeamId(IntegerToGenericTeamId(Teams->FindTeamFromObject(GetInstigator())));
	}

	FHitResult Ground;
	const FVector Origin = GetActorLocation();
	if (Settings.bFallToGround)
	{
		// Surface contact can activate immediately; an airborne spawn must retain its explosion position.
		if (ArenaGroundArea::Trace(GetWorld(), Origin + FVector::UpVector * 2.f, Origin - FVector::UpVector * 2.f, this, Ground))
		{
			Land(Ground);
		}
		else
		{
			Placement.Phase = EArenaGroundAreaPhase::Falling;
			Placement.FallOrigin = Origin;
			Placement.FallStartServerTime = GetServerTime();
			ForceNetUpdate();
		}

		return;
	}

	if (!ArenaGroundArea::Trace(GetWorld(), Origin + FVector::UpVector * 2.f,
			Origin - FVector::UpVector * Settings.GroundTraceDepth, this, Ground))
	{
		Destroy();
		return;
	}

	Land(Ground);
}

void AArenaGroundAreaEffect::Land(const FHitResult& Ground)
{
	if (Ground.bStartPenetrating || Ground.ImpactNormal.Z < FMath::Cos(FMath::DegreesToRadians(Settings.SurfaceMaxAngle)))
	{
		Destroy();
		return;
	}

	Placement.Phase = EArenaGroundAreaPhase::Grounded;
	Placement.GroundLocation = Ground.ImpactPoint + FVector::UpVector * 2.f;
	OnRep_Placement();
	ForceNetUpdate();
	MaxCellLen = FMath::Clamp(FMath::FloorToInt(Settings.Radius / Settings.CellSize), 1, 20);
	PendingCells.Add({FIntPoint::ZeroValue, FIntPoint::ZeroValue, false});
	ExpireTime = GetWorld()->GetTimeSeconds() + Settings.Lifetime;
}

float AArenaGroundAreaEffect::GetServerTime() const
{
	const AGameStateBase* State = GetWorld()->GetGameState();
	return State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}

FVector AArenaGroundAreaEffect::GetFallingLocation() const
{
	const float Elapsed = FMath::Max(0.f, GetServerTime() - Placement.FallStartServerTime);
	const float Accelerating = FMath::Min(Elapsed, Settings.TerminalFallSpeed / Settings.FallAcceleration);
	const float Distance = 0.5f * Settings.FallAcceleration * FMath::Square(Accelerating) + Settings.TerminalFallSpeed * (Elapsed - Accelerating);
	return FVector(Placement.FallOrigin) - FVector::UpVector * FMath::Min(Distance, Settings.GroundTraceDepth);
}

void AArenaGroundAreaEffect::TickFalling()
{
	const FVector Next = GetFallingLocation();
	if (HasAuthority())
	{
		FHitResult Hit;
		// Sweep every traversed vertical segment so thin platforms and low frame rates cannot be skipped.
		if (ArenaGroundArea::Trace(GetWorld(), GetActorLocation() + FVector::UpVector * 2.f, Next, this, Hit))
		{
			Land(Hit);
			return;
		}

		if (Next.Z <= Placement.FallOrigin.Z - Settings.GroundTraceDepth)
		{
			Destroy();
			return;
		}
	}

	SetActorLocation(Next);
}

void AArenaGroundAreaEffect::OnRep_Placement()
{
	if (Placement.Phase != EArenaGroundAreaPhase::Falling)
	{
		ClearFallingVisual(Placement.Phase == EArenaGroundAreaPhase::Grounded);
	}

	if (Placement.Phase == EArenaGroundAreaPhase::Grounded)
	{
		SetActorLocation(Placement.GroundLocation);
	}

	if (Placement.Phase == EArenaGroundAreaPhase::Stopped)
	{
		ClearVisuals();
	}
}

void AArenaGroundAreaEffect::UpdateFallingVisual()
{
	if (!IsFalling() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (!IsValid(FallingVisual) && Settings.FallingEffect)
	{
		FallingVisual = UNiagaraFunctionLibrary::SpawnSystemAttached(Settings.FallingEffect, GetRootComponent(), NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, false);
		if (FallingVisual)
		{
			FallingVisual->SetRelativeScale3D(FVector(Settings.FallingEffectScale));
			FallingMaterial = BindAreaMaterial(FallingVisual, Settings.FallingEffect, GetActorLocation(), Settings.Radius);
			if (const UArenaTeamSubsystem* Teams = GetWorld()->GetSubsystem<UArenaTeamSubsystem>())
			{
				FallingVisual->SetVariableLinearColor(TEXT("Team_Color"), Teams->FindTeamColorFromActor(this));
			}

			FallingVisual->Activate(true);
		}
	}

	if (FallingMaterial)
	{
		const FVector Center = GetActorLocation();
		FallingMaterial->SetVectorParameterValue(TEXT("GroundAreaCenter"), FLinearColor(Center.X, Center.Y, Center.Z));
	}
}

void AArenaGroundAreaEffect::ClearFallingVisual(bool bFinishParticles)
{
	if (IsValid(FallingVisual))
	{
		if (bFinishParticles)
		{
			// Stop emitting at contact while existing world-space particles finish above the ground.
			FallingVisual->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			FallingVisual->SetAutoDestroy(true);
			FallingVisual->Deactivate();
		}
		else
		{
			FallingVisual->DestroyComponent();
		}
	}

	FallingVisual = nullptr;
	FallingMaterial = nullptr;
}

void AArenaGroundAreaEffect::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(EffectTimer);
	RemoveDepartedEffects(true);
	ClearVisuals();
	ClearFallingVisual();
	Super::EndPlay(EndPlayReason);
}

void AArenaGroundAreaEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsFalling())
	{
		TickFalling();
	}

	if (IsActorBeingDestroyed())
	{
		return;
	}

	if (HasAuthority() && Placement.Phase == EArenaGroundAreaPhase::Grounded && !bStopped)
	{
		if (GetWorld()->GetTimeSeconds() >= ExpireTime)
		{
			StopArea();
		}
		else if (!bDiffusionComplete)
		{
			TickDiffusion();
		}
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		UpdateFallingVisual();
		if (Placement.Phase == EArenaGroundAreaPhase::Grounded)
		{
			UpdateVisuals();
		}
	}
}

void AArenaGroundAreaEffect::TickDiffusion()
{
	static const FIntPoint Directions[] = {FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1)};
	int32 Checked = 0;
	while (QueueHead < PendingCells.Num() && Checked < Settings.CellsPerFrame)
	{
		const FPendingCell Pending = PendingCells[QueueHead++];
		const FArenaGroundCell* Existing = Cells.Find(Pending.Coordinate);
		if (Existing && Existing->IsActive())
		{
			continue;
		}

		++Checked;
		FArenaGroundCell Cell;
		Cell.Coordinate = Pending.Coordinate;
		const bool bSuccess = CheckCell(Pending, Cell);
		if (bSuccess)
		{
			Cell.State = EArenaGroundCellState::Active;
		}

		Cells.Add(Pending.Coordinate, Cell);
		if (!bSuccess)
		{
			continue;
		}

		for (const FIntPoint Direction : Directions)
		{
			const FIntPoint Next = Pending.Coordinate + Direction;
			if (FMath::Abs(Next.X) > MaxCellLen || FMath::Abs(Next.Y) > MaxCellLen ||
				FMath::Square(Next.X * Settings.CellSize) + FMath::Square(Next.Y * Settings.CellSize) >= FMath::Square(Settings.Radius))
			{
				continue;
			}

			const FArenaGroundCell* Neighbor = Cells.Find(Next);
			if (Neighbor && Neighbor->IsActive())
			{
				continue;
			}

			// Each active parent expands once. Failed connections can retry from another parent.
			PendingCells.Add({Next, Pending.Coordinate, true});
		}
	}

	if (QueueHead >= PendingCells.Num())
	{
		FinishDiffusion();
	}
}

float AArenaGroundAreaEffect::TraceDown(FVector Center, float MinOffset, FHitResult& Hit) const
{
	const float Top = FMath::Max(Settings.StepHeight, Settings.EffectHeight);
	FVector Start = Center + FVector::UpVector * Top;
	FHitResult Ceiling;
	if (ArenaGroundArea::Trace(GetWorld(), Center, Start, this, Ceiling))
	{
		Start.Z = Ceiling.ImpactPoint.Z - 1.f;
	}

	const FVector End = Center + FVector::UpVector * MinOffset;
	if (Start.Z <= End.Z || !ArenaGroundArea::Trace(GetWorld(), Start, End, this, Hit) ||
		Hit.bStartPenetrating || Hit.ImpactNormal.Z < FMath::Cos(FMath::DegreesToRadians(Settings.SurfaceMaxAngle)))
	{
		return -1.f;
	}

	return FMath::Max(0.f, static_cast<float>(Start.Z - Hit.ImpactPoint.Z));
}

bool AArenaGroundAreaEffect::ParentConnected(const FArenaGroundCell& Parent, FVector Location, float Height) const
{
	const double Offset = Location.Z - GetActorLocation().Z;
	if (Offset > Settings.MaxRise || Offset < -Settings.MaxDrop)
	{
		return false;
	}

	const double MinZ = FMath::Max(Parent.Location.Z, Location.Z) + 2.5;
	const double MaxZ = FMath::Min(Parent.Location.Z + Parent.DiffuseHeight, Location.Z + Height) - 2.5;
	if (MinZ > MaxZ)
	{
		return false;
	}

	for (int32 Layer = 0; Layer < 3; ++Layer)
	{
		const double Z = MinZ + (MaxZ - MinZ) * Layer / 4.;
		FHitResult Hit;
		if (!ArenaGroundArea::Trace(GetWorld(), FVector(Parent.Location.X, Parent.Location.Y, Z),
				FVector(Location.X, Location.Y, Z), this, Hit))
		{
			return true;
		}
	}

	return false;
}

bool AArenaGroundAreaEffect::CheckCell(const FPendingCell& Pending, FArenaGroundCell& OutCell) const
{
	const FArenaGroundCell* Parent = Pending.bHasParent ? Cells.Find(Pending.Parent) : nullptr;
	if (Pending.bHasParent && (!Parent || Parent->State != EArenaGroundCellState::Active))
	{
		return false;
	}

	FVector Center = GetActorLocation() + GetActorQuat().RotateVector(
		FVector(Pending.Coordinate.X * Settings.CellSize, Pending.Coordinate.Y * Settings.CellSize, 0.f));
	if (Parent)
	{
		Center.Z = Parent->Location.Z + FMath::Min(Parent->EffectHeight, 3.f);
	}

	FHitResult Hit;
	float Height = TraceDown(Center, -Settings.MaxDrop, Hit);
	if (Parent)
	{
		if (Height >= 0.f && !ParentConnected(*Parent, Hit.ImpactPoint, Height))
		{
			Height = -1.f;
		}

		if (Height < Settings.ChildLayerMinHeight)
		{
			FHitResult StepHit;
			const float StepHeight = TraceDown(Center + FVector::UpVector * Parent->DiffuseHeight, -Parent->DiffuseHeight, StepHit);
			if (StepHeight > Height && StepHeight > 0.f && ParentConnected(*Parent, StepHit.ImpactPoint, StepHeight))
			{
				Height = StepHeight;
				Hit = StepHit;
			}
		}
	}

	if (Height <= Settings.MinClearance)
	{
		return false;
	}

	OutCell.Location = Hit.ImpactPoint;
	OutCell.Normal = Hit.ImpactNormal;
	OutCell.EffectHeight = FMath::Min(Settings.EffectHeight, Height);
	OutCell.DiffuseHeight = FMath::Min(Settings.StepHeight, Height);
	return true;
}

void AArenaGroundAreaEffect::FinishDiffusion()
{
	bDiffusionComplete = true;
	PendingCells.Empty();
	SyncActiveCells();
	if (ActiveCells.IsEmpty())
	{
		StopArea();
		return;
	}

	GetWorldTimerManager().SetTimer(EffectTimer, this, &ThisClass::EffectTick, Settings.ApplicationInterval, true);
}

bool AArenaGroundAreaEffect::IsInArea(const FVector& CapsuleCenter, float Radius, float HalfHeight) const
{
	if (!HasAuthority() || bStopped || Placement.Phase != EArenaGroundAreaPhase::Grounded)
	{
		return false;
	}

	const FQuat InverseYaw = GetActorQuat().Inverse();
	for (const TPair<FIntPoint, FArenaGroundCell>& Pair : Cells)
	{
		const FArenaGroundCell& Cell = Pair.Value;
		if (!Cell.IsActive() || Cell.EffectHeight <= 0.f)
		{
			continue;
		}

		const FVector Delta = CapsuleCenter - Cell.Location;
		const FVector LocalDelta = InverseYaw.RotateVector(Delta);
		const double Dxy = FMath::Max(FMath::Abs(LocalDelta.X), FMath::Abs(LocalDelta.Y));
		const double OutsideX = FMath::Max(0., FMath::Abs(LocalDelta.X) - Settings.CellSize * 0.5f);
		const double OutsideY = FMath::Max(0., FMath::Abs(LocalDelta.Y) - Settings.CellSize * 0.5f);
		if (FMath::Square(OutsideX) + FMath::Square(OutsideY) > FMath::Square(Radius) || Delta.Z < -HalfHeight * 0.5f ||
			Delta.Z > HalfHeight + Cell.EffectHeight - 5.f - Dxy)
		{
			continue;
		}

		// A square cell can extend past a thin wall, even when it overlaps the capsule.
		const FVector CellCenter = Cell.Location + FVector::UpVector * Cell.EffectHeight * 0.5f;
		FHitResult Hit;
		if (!ArenaGroundArea::Trace(GetWorld(), CapsuleCenter, CellCenter, this, Hit))
		{
			return true;
		}
	}

	return false;
}

bool AArenaGroundAreaEffect::CanAffectTarget(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	if (Target == GetInstigator())
	{
		return Settings.bAffectInstigator;
	}

	const UArenaTeamSubsystem* Teams = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
	const EArenaTeamComparison Comparison = Teams ? Teams->CompareTeams(this, Target) : EArenaTeamComparison::InvalidArgument;
	return Comparison == EArenaTeamComparison::OnSameTeam ? Settings.bAffectAllies : Settings.bAffectEnemies;
}

bool AArenaGroundAreaEffect::ContainsTarget(AActor* Target) const
{
	if (!CanAffectTarget(Target))
	{
		return false;
	}

	float Radius = 0.f;
	float HalfHeight = 0.f;
	Target->GetSimpleCollisionCylinder(Radius, HalfHeight);
	FVector Center = Target->GetActorLocation();
	if (const ACharacter* Character = Cast<ACharacter>(Target))
	{
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		Capsule->GetScaledCapsuleSize(Radius, HalfHeight);
		Center = Capsule->GetComponentLocation();
	}

	return IsInArea(Center, Radius, HalfHeight);
}

void AArenaGroundAreaEffect::RemoveDepartedEffects(bool bRemoveAll)
{
	for (auto It = AppliedEffects.CreateIterator(); It; ++It)
	{
		UAbilitySystemComponent* TargetASC = It.Value().Target.Get();
		const bool bActive = TargetASC && TargetASC->GetActiveGameplayEffect(It.Value().Handle);
		// Lingering effects retain their handle outside the area, so reentry cannot add a duplicate.
		if (bRemoveAll || !It.Key().IsValid() || !bActive ||
			(Settings.bRemoveEffectOnExit && !ContainsTarget(It.Key().Get())))
		{
			if (Settings.bRemoveEffectOnExit && TargetASC)
			{
				TargetASC->RemoveActiveGameplayEffect(It.Value().Handle, 1);
			}

			It.RemoveCurrent();
		}
	}
}

void AArenaGroundAreaEffect::PrepareGameplayEffect()
{
	UAbilitySystemComponent* Source = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());
	if (!Source || !Settings.GameplayEffect)
	{
		return;
	}

	FGameplayEffectContextHandle Context = Source->MakeEffectContext();
	Context.Get()->SetEffectCauser(this);
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(Settings.GameplayEffect, Settings.EffectLevel, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	for (const TPair<FGameplayTag, float>& Magnitude : Settings.SetByCallerMagnitudes)
	{
		Spec.Data->SetSetByCallerMagnitude(Magnitude.Key, Magnitude.Value);
	}

	for (const FGameplayTag& Tag : Settings.EffectAssetTags)
	{
		Spec.Data->AddDynamicAssetTag(Tag);
	}

	PreparedEffectSpec = *Spec.Data;
}

void AArenaGroundAreaEffect::EffectTick()
{
	if (!HasAuthority() || bStopped || GetWorld()->GetTimeSeconds() >= ExpireTime)
	{
		return;
	}

	RemoveDepartedEffects();
	if (!PreparedEffectSpec.Def)
	{
		return;
	}

	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(GroundAreaEffect), false, this);
	const float Top = Settings.MaxRise + Settings.EffectHeight;
	const FVector Center = GetActorLocation() + FVector::UpVector * (Top - Settings.MaxDrop) * 0.5f;
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Objects,
		FCollisionShape::MakeBox(FVector(Settings.Radius + Settings.CellSize * 0.5f,
			Settings.Radius + Settings.CellSize * 0.5f, (Top + Settings.MaxDrop) * 0.5f)),
		Query);
	TSet<AActor*> Visited;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!IsValid(Target) || Visited.Contains(Target))
		{
			continue;
		}

		Visited.Add(Target);
		if (!ContainsTarget(Target))
		{
			continue;
		}

		UAbilitySystemComponent* Destination = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		if (!Destination)
		{
			continue;
		}

		if (const FAppliedAreaEffect* Applied = AppliedEffects.Find(Target); Applied && Destination->GetActiveGameplayEffect(Applied->Handle))
		{
			continue;
		}

		const FActiveGameplayEffectHandle Handle = Destination->ApplyGameplayEffectSpecToSelf(PreparedEffectSpec);
		if (Handle.IsValid())
		{
			AppliedEffects.Add(Target, {Destination, Handle});
		}
	}
}

void AArenaGroundAreaEffect::SyncActiveCells()
{
	ActiveCells.Reset();
	TMap<int32, FArenaGroundVisualCell> Merged;
	int32 ActiveCount = 0;
	bool bFull = Settings.FullAreaEffect != nullptr;
	const FArenaGroundCell* Center = Cells.Find(FIntPoint::ZeroValue);
	for (const TPair<FIntPoint, FArenaGroundCell>& Pair : Cells)
	{
		const FArenaGroundCell& Cell = Pair.Value;
		if (!Cell.IsActive())
		{
			continue;
		}

		++ActiveCount;
		bFull &= Center && FMath::Abs(Cell.Location.Z - Center->Location.Z) <= 100.f &&
			FVector::DotProduct(Cell.Normal, Center->Normal) >= FMath::Cos(FMath::DegreesToRadians(30.f));
		const int32 X = FMath::FloorToInt((Cell.Coordinate.X + Settings.ClientCellMerge * 0.5f) / Settings.ClientCellMerge);
		const int32 Y = FMath::FloorToInt((Cell.Coordinate.Y + Settings.ClientCellMerge * 0.5f) / Settings.ClientCellMerge);
		const int32 Index = (X + 100) + (Y + 100) * 1000;
		FArenaGroundVisualCell* Existing = Merged.Find(Index);
		const float Height = Cell.Location.Z - GetActorLocation().Z + Cell.EffectHeight;
		const FVector Ideal = GetActorLocation() + GetActorQuat().RotateVector(
			FVector(X * Settings.ClientCellMerge * Settings.CellSize, Y * Settings.ClientCellMerge * Settings.CellSize, 0.f));
		if (!Existing)
		{
			FArenaGroundVisualCell Visual;
			Visual.Index = Index;
			Visual.MaxHeight = Height;
			Visual.Location = Cell.Location;
			Visual.Normal = Cell.Normal;
			Merged.Add(Index, Visual);
		}
		else
		{
			Existing->MaxHeight = FMath::Max(Existing->MaxHeight, Height);
			if (FVector::DistSquared2D(Cell.Location, Ideal) < FVector::DistSquared2D(Existing->Location, Ideal))
			{
				Existing->Location = Cell.Location;
				Existing->Normal = Cell.Normal;
			}
		}
	}

	const float Theoretical = FMath::Square(2.f * MaxCellLen) * PI / 4.f;
	if (bFull && ActiveCount >= Theoretical * 0.8f && Center && Center->EffectHeight > 0.f)
	{
		FArenaGroundVisualCell Full;
		Full.Index = -1;
		Full.Location = Center->Location;
		Full.Normal = Center->Normal;
		Full.MaxHeight = Center->EffectHeight;
		ActiveCells.Add(Full);
	}
	else
	{
		Merged.GenerateValueArray(ActiveCells);
		ActiveCells.Sort([](const FArenaGroundVisualCell& A, const FArenaGroundVisualCell& B)
			{
				return A.Index < B.Index;
			});
	}

	if (ActiveCells.Num() > Settings.MaxVisualCount)
	{
		TArray<FArenaGroundVisualCell> Sampled;
		for (int32 I = 0; I < Settings.MaxVisualCount; ++I)
		{
			Sampled.Add(ActiveCells[(I * ActiveCells.Num() + ActiveCells.Num() / 2) / Settings.MaxVisualCount]);
		}

		ActiveCells = MoveTemp(Sampled);
	}

	ForceNetUpdate();
	OnRep_ActiveCells(); // RepNotifies do not run automatically on a listen server.
}

void AArenaGroundAreaEffect::StopArea()
{
	if (bStopped)
	{
		return;
	}

	bStopped = true;
	Placement.Phase = EArenaGroundAreaPhase::Stopped;
	GetWorldTimerManager().ClearTimer(EffectTimer);
	RemoveDepartedEffects(true);
	ActiveCells.Empty();
	ForceNetUpdate();
	OnRep_ActiveCells();
	SetLifeSpan(FMath::Max(0.01f, Settings.DestroyDelay));
}

void AArenaGroundAreaEffect::OnRep_ActiveCells()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	VisualCursor = 0;
	bVisualsDirty = true;
	TSet<int32> Wanted;
	for (const FArenaGroundVisualCell& Cell : ActiveCells)
	{
		Wanted.Add(Cell.Index);
	}

	for (auto It = Visuals.CreateIterator(); It; ++It)
	{
		if (!Wanted.Contains(It.Key()))
		{
			if (IsValid(It.Value()))
			{
				It.Value()->DestroyComponent();
			}

			It.RemoveCurrent();
		}
	}
}

FTransform AArenaGroundAreaEffect::GroundFit(const FArenaGroundVisualCell& Cell) const
{
	const FVector Representative = Cell.Location;
	// Keep probes near the actual connected logical cell, including on stacked surfaces.
	const float Offset = FMath::Min(30.f, Settings.CellSize * 0.4f);
	const FVector Offsets[] = {FVector::ZeroVector, FVector(Offset, Offset, 0), FVector(-Offset, Offset, 0),
		FVector(Offset, -Offset, 0), FVector(-Offset, -Offset, 0)};
	TArray<FHitResult, TInlineAllocator<5>> Hits;
	double WeightedHeight = 0.;
	double Weight = 0.;
	for (int32 I = 0; I < 5; ++I)
	{
		const FVector Point = Representative + Offsets[I];
		FHitResult Hit;
		if (ArenaGroundArea::Trace(GetWorld(), Point + FVector::UpVector * 10.f, Point - FVector::UpVector * 60.f, this, Hit) &&
			Hit.ImpactNormal.Z >= 0.2f && FMath::Abs(Hit.ImpactPoint.Z - Representative.Z) < 30.)
		{
			const double W = I == 0 ? 2. : 1.;
			WeightedHeight += Hit.ImpactPoint.Z * W;
			Weight += W;
			Hits.Add(Hit);
		}
	}

	FVector Location = Representative;
	FVector Normal = Cell.Normal;
	if (Weight > 0.)
	{
		const double Average = WeightedHeight / Weight;
		Hits.Sort([Average](const FHitResult& A, const FHitResult& B)
			{
				return FMath::Abs(A.ImpactPoint.Z - Average) < FMath::Abs(B.ImpactPoint.Z - Average);
			});
		Location.Z = Hits[0].ImpactPoint.Z;
		Normal = Hits[0].ImpactNormal;
	}

	return FTransform(FRotationMatrix::MakeFromZX(Normal, GetActorForwardVector()).ToQuat(),
		Location + Normal * 2.f, FVector(Settings.EffectScale));
}

void AArenaGroundAreaEffect::UpdateVisuals()
{
	if (Settings.bDrawDebugCells && HasAuthority())
	{
		for (const TPair<FIntPoint, FArenaGroundCell>& Pair : Cells)
		{
			const FArenaGroundCell& Cell = Pair.Value;
			if (Cell.State != EArenaGroundCellState::Active)
			{
				continue;
			}

			DrawDebugBox(GetWorld(), Cell.Location + FVector::UpVector * Cell.EffectHeight * 0.5f,
				FVector(Settings.CellSize * 0.5f, Settings.CellSize * 0.5f, FMath::Max(1.f, Cell.EffectHeight * 0.5f)),
				GetActorQuat(), FColor::Orange, false, -1.f);
		}
	}

	if (!bVisualsDirty)
	{
		return;
	}

	int32 Created = 0;
	while (VisualCursor < ActiveCells.Num() && Created++ < Settings.VisualsPerFrame)
	{
		const FArenaGroundVisualCell& Cell = ActiveCells[VisualCursor++];
		UNiagaraSystem* Effect = Cell.Index == -1 ? Settings.FullAreaEffect : Settings.CellEffect;
		const FTransform Transform = GroundFit(Cell);
		UNiagaraComponent* Existing = Visuals.FindRef(Cell.Index);
		if (IsValid(Existing))
		{
			Existing->SetWorldTransform(Transform);
		}
		else if (Effect)
		{
			UNiagaraComponent* Visual = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Effect,
				Transform.GetLocation(), Transform.Rotator(), Transform.GetScale3D(), false, false);
			if (Visual)
			{
				BindAreaMaterial(Visual, Effect, GetActorLocation(), Settings.Radius);
				if (const UArenaTeamSubsystem* Teams = GetWorld()->GetSubsystem<UArenaTeamSubsystem>())
				{
					Visual->SetVariableLinearColor(TEXT("Team_Color"), Teams->FindTeamColorFromActor(this));
				}

				Visual->Activate(true);
				Visuals.Add(Cell.Index, Visual);
			}
		}
	}

	bVisualsDirty = VisualCursor < ActiveCells.Num();
}

void AArenaGroundAreaEffect::ClearVisuals()
{
	for (const TPair<int32, TObjectPtr<UNiagaraComponent>>& Pair : Visuals)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->DestroyComponent();
		}
	}

	Visuals.Empty();
}
