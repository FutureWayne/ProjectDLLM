// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/AbilityTask/TracePrevisionTrajectory.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"



UTracePrevisionTrajectory::UTracePrevisionTrajectory(const FObjectInitializer& ObjectInitializer)
{
	bTickingTask = true;
}

UTracePrevisionTrajectory* UTracePrevisionTrajectory::StartTracing(UGameplayAbility* OwningAbility, FGetLocationDelegate StartLocationDelegate, FGetVelocityDelegate
                                                                   LaunchVelocityDelegate, UNiagaraSystem* TossAimTrailEffect)
{
	UTracePrevisionTrajectory* Task = NewAbilityTask<UTracePrevisionTrajectory>(OwningAbility);
	Task->GetStartLocation = StartLocationDelegate;
	Task->GetLaunchVelocity = LaunchVelocityDelegate;
	Task->OwnerActor = OwningAbility->GetCurrentActorInfo()->AvatarActor.Get();

	if (TossAimTrailEffect)
	{
		Task->TrailEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(TossAimTrailEffect, Task->OwnerActor->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	}
	
	return Task;
}

void UTracePrevisionTrajectory::Activate()
{
	Super::Activate();
	
	SetWaitingOnAvatar();
}

void UTracePrevisionTrajectory::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		// Get the updated StartLocation and LaunchVelocity from the delegates
		FVector StartLocation = GetStartLocation.Execute();
		FVector LaunchVelocity = GetLaunchVelocity.Execute();

		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(OwnerActor.Get());
		
		// Use PredictProjectilePath to calculate the trajectory
		FPredictProjectilePathParams PathParams;
		PathParams.StartLocation = StartLocation;
		PathParams.LaunchVelocity = LaunchVelocity;
		PathParams.ProjectileRadius = 5.f; // Adjust as needed
		PathParams.bTraceWithCollision = true;
		PathParams.OverrideGravityZ = 0.f; // Default gravity
		PathParams.ActorsToIgnore = ActorsToIgnore;

		FPredictProjectilePathResult PathResult;
		UGameplayStatics::PredictProjectilePath(GetWorld(), PathParams, PathResult);

		// Extract trajectory points
		TArray<FVector> TrajectoryPoints;
		for (const FPredictProjectilePathPointData& Point : PathResult.PathData)
		{
			TrajectoryPoints.Add(Point.Location);
		}

		// Broadcast the trajectory points
		OnTrajectoryUpdated.Broadcast(TrajectoryPoints, TrailEffectComponent);
	}
}

void UTracePrevisionTrajectory::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);

	if (TrailEffectComponent)
	{
		TrailEffectComponent->Deactivate();
	}
}
