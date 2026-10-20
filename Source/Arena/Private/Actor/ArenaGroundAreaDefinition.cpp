// Copyright Ludens Studio. All Rights Reserved.

#include "Actor/ArenaGroundAreaDefinition.h"

#include "GameplayEffect.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void FArenaGroundAreaSettings::Sanitize()
{
	CellSize = FMath::Clamp(CellSize, 10.f, 600.f);
	Radius = FMath::Clamp(Radius, CellSize, 20.f * CellSize);
	CellsPerFrame = FMath::Clamp(CellsPerFrame, 1, 512);
	Lifetime = FMath::Max(0.1f, Lifetime);
	DestroyDelay = FMath::Max(0.f, DestroyDelay);
	ApplicationInterval = FMath::Max(0.05f, ApplicationInterval);
	EffectLevel = FMath::Max(0.01f, EffectLevel);
	GroundTraceDepth = FMath::Max(1.f, GroundTraceDepth);
	FallAcceleration = FMath::Max(1.f, FallAcceleration);
	TerminalFallSpeed = FMath::Max(1.f, TerminalFallSpeed);
	FallingEffectScale = FMath::Max(0.01f, FallingEffectScale);
	MaxRise = FMath::Max(0.f, MaxRise);
	MaxDrop = FMath::Max(0.f, MaxDrop);
	StepHeight = FMath::Max(6.f, StepHeight);
	EffectHeight = FMath::Max(1.f, EffectHeight);
	SurfaceMaxAngle = FMath::Clamp(SurfaceMaxAngle, 0.f, 89.f);
	ChildLayerMinHeight = FMath::Max(0.f, ChildLayerMinHeight);
	MinClearance = FMath::Max(0.f, MinClearance);
	ClientCellMerge = FMath::Clamp(ClientCellMerge, 1, 10);
	MaxVisualCount = FMath::Clamp(MaxVisualCount, 1, 512);
	VisualsPerFrame = FMath::Clamp(VisualsPerFrame, 1, 32);
	EffectScale = FMath::Max(0.01f, EffectScale);
}

#if WITH_EDITOR
EDataValidationResult UArenaGroundAreaDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);
	if (!Settings.GameplayEffect || Settings.GameplayEffect->HasAnyClassFlags(CLASS_Abstract))
	{
		Context.AddError(NSLOCTEXT("Arena", "AreaEffectMissing", "Ground area requires a non-abstract GameplayEffect."));
		Result = EDataValidationResult::Invalid;
	}

	if (Settings.Radius < Settings.CellSize || Settings.CellSize < 10.f || Settings.CellSize > 600.f ||
		Settings.Radius > 20.f * Settings.CellSize || Settings.Lifetime <= 0.f || Settings.ApplicationInterval < 0.05f ||
		Settings.GroundTraceDepth <= 0.f || Settings.MaxVisualCount < 1 || Settings.MaxVisualCount > 512 ||
		(Settings.bFallToGround && (Settings.FallAcceleration <= 0.f || Settings.TerminalFallSpeed <= 0.f || Settings.FallingEffectScale <= 0.f)))
	{
		Context.AddError(NSLOCTEXT("Arena", "AreaLimitsInvalid", "Ground area coverage, lifetime, interval, projection depth or visual count is outside supported limits."));
		Result = EDataValidationResult::Invalid;
	}

	if (Settings.GameplayEffect && Settings.GameplayEffect->GetDefaultObject<UGameplayEffect>()->DurationPolicy == EGameplayEffectDurationType::Infinite && !Settings.bRemoveEffectOnExit)
	{
		Context.AddWarning(NSLOCTEXT("Arena", "AreaInfinitePersists", "This infinite effect will remain after leaving or destroying the area. Enable RemoveEffectOnExit for area-only effects."));
	}

	return Result;
}
#endif
