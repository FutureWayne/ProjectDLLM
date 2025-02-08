// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "TracePrevisionTrajectory.generated.h"

// Delegate definitions
DECLARE_DYNAMIC_DELEGATE_RetVal(FVector, FGetLocationDelegate);
DECLARE_DYNAMIC_DELEGATE_RetVal(FRotator, FGetDirectionDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrajectoryUpdated, const TArray<FVector>&, TrajectoryPoints);

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
	static UTracePrevisionTrajectory* StartTracing(UGameplayAbility* OwningAbility, FGetLocationDelegate StartLocationDelegate, FGetDirectionDelegate LaunchDirectionDelegate, float LaunchSpeed, float GravityScale);

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
	FGetDirectionDelegate GetLaunchDirection;
	float LaunchSpeed = 0.0f;
	float GravityScale = 0.0f;

	UPROPERTY()
	TObjectPtr<AActor> OwnerActor;
};
