// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "TracePrevisionTrajectory.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

// Delegate definitions
DECLARE_DYNAMIC_DELEGATE_RetVal(FVector, FGetLocationDelegate);
DECLARE_DYNAMIC_DELEGATE_RetVal(FVector, FGetVelocityDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTrajectoryUpdated, const TArray<FVector>&, TrajectoryPoints, UNiagaraComponent*, TrailEffectComponent);

/**
 * 
 */
UCLASS()
class ARENA_API UTracePrevisionTrajectory : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	UTracePrevisionTrajectory(const FObjectInitializer& ObjectInitializer);
	
	// Factory method to start the task with dynamic delegates
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (DisplayName = "Trace Prevision Trajectory", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UTracePrevisionTrajectory* StartTracing(UGameplayAbility* OwningAbility, FGetLocationDelegate StartLocationDelegate, FGetVelocityDelegate LaunchVelocityDelegate, UNiagaraSystem* TossAimTrailEffect);

	// Event triggered when the trajectory is updated
	UPROPERTY(BlueprintAssignable)
	FOnTrajectoryUpdated OnTrajectoryUpdated;

protected:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	// Delegates for retrieving StartLocation and LaunchVelocity
	FGetLocationDelegate GetStartLocation;
	FGetVelocityDelegate GetLaunchVelocity;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailEffectComponent;

	UPROPERTY()
	TObjectPtr<AActor> OwnerActor;
};
