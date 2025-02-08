// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/AbilityTask/TracePrevisionTrajectory.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"

UTracePrevisionTrajectory::UTracePrevisionTrajectory(const FObjectInitializer& ObjectInitializer)
{
	bTickingTask = true;
}

UTracePrevisionTrajectory* UTracePrevisionTrajectory::StartTracing(UGameplayAbility* OwningAbility, FGetLocationDelegate StartLocationDelegate, FGetDirectionDelegate
                                                                   LaunchDirectionDelegate, float LaunchSpeed, float GravityScale)
{
	UTracePrevisionTrajectory* Task = NewAbilityTask<UTracePrevisionTrajectory>(OwningAbility);
	Task->GetStartLocation = StartLocationDelegate;
	Task->GetLaunchDirection = LaunchDirectionDelegate;
	Task->LaunchSpeed = LaunchSpeed;
	Task->GravityScale = GravityScale;
	
	Task->OwnerActor = OwningAbility->GetCurrentActorInfo()->AvatarActor.Get();
	
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
		FRotator LaunchDirection = GetLaunchDirection.Execute();

		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(OwnerActor.Get());

		FVector LaunchVelocity = LaunchDirection.Vector() * LaunchSpeed;
		
		// Use PredictProjectilePath to calculate the trajectory
		FPredictProjectilePathParams PathParams;
		PathParams.StartLocation = StartLocation;
		PathParams.LaunchVelocity = LaunchVelocity;
		PathParams.ProjectileRadius = 5.f; // Adjust as needed
		PathParams.bTraceWithCollision = true;
		PathParams.OverrideGravityZ = -980 * GravityScale - 0.1f; // Adjust gravity scale;
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
		OnTrajectoryUpdated.Broadcast(TrajectoryPoints);
	}
}

void UTracePrevisionTrajectory::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
}
