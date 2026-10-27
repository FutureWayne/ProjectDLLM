// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Actor/ArenaEffectActorDefinition.h"
#include "GameplayTagContainer.h"
#include "ArenaGroundAreaDefinition.generated.h"

class UNiagaraSystem;
class UGameplayEffect;

/** Surface coverage, gameplay and presentation are independent. Distances are in cm. */
USTRUCT(BlueprintType)
struct ARENA_API FArenaGroundAreaSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "10"))
	float Radius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "10"))
	float CellSize = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "1", ClampMax = "512"))
	int32 CellsPerFrame = 128;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "0.1"))
	float Lifetime = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "0"))
	float DestroyDelay = 1.5f;

	/** Instant effects repeat at this interval. Active duration/infinite effects are not stacked every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0.05"))
	float ApplicationInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (AllowAbstract = "false"))
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (ClampMin = "0.01"))
	float EffectLevel = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, NotReplicated, Category = "Gameplay")
	TMap<FGameplayTag, float> SetByCallerMagnitudes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	FGameplayTagContainer EffectAssetTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
	bool bRemoveEffectOnExit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Targets")
	bool bAffectInstigator = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Targets")
	bool bAffectEnemies = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Targets")
	bool bAffectAllies = false;

	/** Optional airborne stage: fall vertically, then begin coverage and lifetime on surface contact. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Falling")
	bool bFallToGround = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Falling", meta = (ClampMin = "1"))
	float FallAcceleration = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|Falling", meta = (ClampMin = "1"))
	float TerminalFallSpeed = 2400.f;

	/** Maximum projection or falling distance. No valid surface means no active area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface", meta = (ClampMin = "1"))
	float GroundTraceDepth = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface", meta = (ClampMin = "0"))
	float MaxRise = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface", meta = (ClampMin = "0"))
	float MaxDrop = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface", meta = (ClampMin = "6"))
	float StepHeight = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface", meta = (ClampMin = "1"))
	float EffectHeight = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface", meta = (ClampMin = "0", ClampMax = "89"))
	float SurfaceMaxAngle = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface", meta = (ClampMin = "0"))
	float ChildLayerMinHeight = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface", meta = (ClampMin = "0"))
	float MinClearance = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UNiagaraSystem> CellEffect = nullptr;

	/** Optional calibrated whole-area system used only on a sufficiently complete, flat area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UNiagaraSystem> FullAreaEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Falling")
	TObjectPtr<UNiagaraSystem> FallingEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1", ClampMax = "10"))
	int32 ClientCellMerge = 6;

	/** Maximum Niagara instances in an area, sampled evenly from valid surface cells. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1", ClampMax = "512"))
	int32 MaxVisualCount = 64;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1", ClampMax = "32"))
	int32 VisualsPerFrame = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebugCells = false;

	void Sanitize();
};

/** Reusable fire, frost, poison, healing, etc. configuration; no grenade dependency. */
UCLASS(BlueprintType)
class ARENA_API UArenaGroundAreaDefinition : public UArenaEffectActorDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground Area")
	FArenaGroundAreaSettings Settings;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
