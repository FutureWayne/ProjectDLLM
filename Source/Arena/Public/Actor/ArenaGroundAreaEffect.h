// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Actor/ArenaTeamEffectActor.h"
#include "Actor/ArenaGroundAreaDefinition.h"
#include "ActiveGameplayEffectHandle.h"
#include "Engine/NetSerialization.h"
#include "Engine/TimerHandle.h"
#include "ArenaGroundAreaEffect.generated.h"

class UNiagaraComponent;
class UAbilitySystemComponent;
class UMaterialInstanceDynamic;

UENUM()
enum class EArenaGroundAreaPhase : uint8
{
	Pending,
	Falling,
	Grounded,
	Stopped
};

/** Replicate transitions, then evaluate the same vertical trajectory on server and clients. */
USTRUCT()
struct FArenaGroundAreaPlacement
{
	GENERATED_BODY()

	UPROPERTY()
	EArenaGroundAreaPhase Phase = EArenaGroundAreaPhase::Pending;

	UPROPERTY()
	FVector_NetQuantize10 FallOrigin = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantize10 GroundLocation = FVector::ZeroVector;

	UPROPERTY()
	float FallStartServerTime = 0.f;
};

enum class EArenaGroundCellState : uint8
{
	Failed,
	Active
};

struct FArenaGroundCell
{
	FIntPoint Coordinate = FIntPoint::ZeroValue;
	FVector Location = FVector::ZeroVector;
	FVector Normal = FVector::UpVector;
	float EffectHeight = 0.f;
	float DiffuseHeight = 0.f;
	EArenaGroundCellState State = EArenaGroundCellState::Failed;

	bool IsActive() const { return State == EArenaGroundCellState::Active; }
};

/** One merged display cell. The representative point prevents visuals across a wall or on another floor. */
USTRUCT()
struct FArenaGroundVisualCell
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Index = 0;

	UPROPERTY()
	float MaxHeight = 0.f;

	UPROPERTY()
	FVector_NetQuantize10 Location = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantizeNormal Normal = FVector::UpVector;
};

/** Server-only surface coverage and gameplay effects; clients receive only display cells. */
UCLASS(Blueprintable)
class ARENA_API AArenaGroundAreaEffect : public AArenaTeamEffectActor
{
	GENERATED_BODY()

public:
	AArenaGroundAreaEffect();

	void Initialize(const FArenaGroundAreaSettings& InSettings);

	virtual void InitializeEffect_Implementation(const UArenaEffectActorDefinition* Definition) override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FOnArenaTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &TeamChangedDelegate; }

	bool IsInArea(const FVector& CapsuleCenter, float Radius, float HalfHeight) const;

	const TMap<FIntPoint, FArenaGroundCell>& GetCells() const { return Cells; }

	bool IsDiffusionComplete() const { return bDiffusionComplete; }

	const TArray<FArenaGroundVisualCell>& GetVisualCells() const { return ActiveCells; }

	const FArenaGroundAreaSettings& GetSettings() const { return Settings; }

	int32 GetSpawnedVisualCount() const { return Visuals.Num(); }

	bool IsFalling() const { return Placement.Phase == EArenaGroundAreaPhase::Falling; }

	UNiagaraComponent* GetFallingVisual() const { return FallingVisual; }

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FPendingCell
	{
		FIntPoint Coordinate;
		FIntPoint Parent;
		bool bHasParent;
	};

	void TickDiffusion();

	void TickFalling();

	float GetServerTime() const;

	FVector GetFallingLocation() const;

	void Land(const FHitResult& Ground);

	void UpdateFallingVisual();

	void ClearFallingVisual(bool bFinishParticles = false);

	UFUNCTION()
	void OnRep_Placement();

	bool CheckCell(const FPendingCell& Pending, FArenaGroundCell& OutCell) const;

	float TraceDown(FVector Center, float MinOffset, FHitResult& Hit) const;

	bool ParentConnected(const FArenaGroundCell& Parent, FVector Location, float Height) const;

	void FinishDiffusion();

	void EffectTick();

	bool CanAffectTarget(AActor* Target) const;

	bool ContainsTarget(AActor* Target) const;

	void RemoveDepartedEffects(bool bRemoveAll = false);

	void SyncActiveCells();

	void StopArea();

	void UpdateVisuals();

	void ClearVisuals();

	FTransform GroundFit(const FArenaGroundVisualCell& Cell) const;

	UFUNCTION()
	void OnRep_ActiveCells();

	UPROPERTY(Replicated)
	FArenaGroundAreaSettings Settings;

	UPROPERTY(ReplicatedUsing = OnRep_Placement)
	FArenaGroundAreaPlacement Placement;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> FallingVisual = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FallingMaterial = nullptr;

	struct FAppliedAreaEffect
	{
		TWeakObjectPtr<UAbilitySystemComponent> Target;
		FActiveGameplayEffectHandle Handle;
	};

	TMap<TWeakObjectPtr<AActor>, FAppliedAreaEffect> AppliedEffects;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveCells)
	TArray<FArenaGroundVisualCell> ActiveCells;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UNiagaraComponent>> Visuals;

	UPROPERTY()
	FOnArenaTeamIndexChangedDelegate TeamChangedDelegate;

	TMap<FIntPoint, FArenaGroundCell> Cells;
	TArray<FPendingCell> PendingCells;
	TSet<FIntPoint> EnqueuedCells;
	FTimerHandle EffectTimer;
	int32 QueueHead = 0;
	int32 MaxCellLen = 0;
	int32 VisualCursor = 0;
	double ExpireTime = 0.;
	bool bDiffusionComplete = false;
	bool bStopped = false;
	bool bVisualsDirty = false;
};
