// Copyright Ludens Studio. All Rights Reserved.


#include "AbilitySystem/Ability/ArenaGameplayAbility_Grenade.h"

#include "ArenaLogChannel.h"
#include "Character/ArenaCharacter.h"
#include "Inventory/ArenaInventoryItemInstance.h"
#include "Inventory/InventoryFragment_GrenadeDef.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/ArenaPlayerController.h"
#include "System/ArenaSystemStatics.h"
#include "Weapon/ArenaGrenadeBase.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

UArenaGameplayAbility_Grenade::UArenaGameplayAbility_Grenade(const FObjectInitializer& ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
}

void UArenaGameplayAbility_Grenade::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UArenaInventoryItemInstance* ItemInstance = GetAssociatedItem();
	if (!ItemInstance)
	{
		UE_LOG(LogArena, Error, TEXT("UArenaGameplayAbility_Grenade::ActivateAbility: ItemInstance is nullptr."));
		return;
	}

	const UInventoryFragment_GrenadeDef* GrenadeDef = ItemInstance->FindFragmentByClass<UInventoryFragment_GrenadeDef>();
	GrenadeDefinitionData = GrenadeDef->GetGrenadeDefinitionData();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

AArenaGrenadeBase* UArenaGameplayAbility_Grenade::SpawnGrenade(FVector SpawnLocation, FRotator SpawnRotation)
{
	const FTransform SpawnTransform = FTransform(SpawnRotation, SpawnLocation);
	AActor* Owner = GetOwningActorFromActorInfo();
	APawn* Instigator = GetArenaCharacterFromActorInfo();
	
	AArenaGrenadeBase* RetGrenade = UArenaSystemStatics::SpawnGrenadeByGrenadeInstance(GetWorld(), SpawnTransform, GrenadeDefinitionData, Owner, Instigator);
	if (ensureMsgf(RetGrenade, TEXT("UArenaGameplayAbility_Grenade::SpawnGrenade: OutGrenade is nullptr.")))
	{
		return RetGrenade;
	}
	
	return nullptr;
}

FVector UArenaGameplayAbility_Grenade::GetSpawnLocation()
{
	AArenaCharacter* ArenaCharacter = GetArenaCharacterFromActorInfo();
	if (ensure(ArenaCharacter))
	{
		return ArenaCharacter->GetActorLocation() + ArenaCharacter->GetActorForwardVector() * SpawnLocationOffset.X + ArenaCharacter->GetActorRightVector() * SpawnLocationOffset.Y + ArenaCharacter->GetActorUpVector() * SpawnLocationOffset.Z;
	}

	return FVector::ZeroVector;
}

FRotator UArenaGameplayAbility_Grenade::GetSpawnRotation()
{
	AArenaPlayerController* ArenaPC = GetArenaPlayerControllerFromActorInfo();
	if (!ensure(ArenaPC))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return FRotator::ZeroRotator;
	}
	
	FVector CameraLocation = ArenaPC->PlayerCameraManager->GetCameraLocation();
	FRotator CameraRotation = ArenaPC->PlayerCameraManager->GetCameraRotation();
	FVector TraceStart = CameraLocation + FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::X) * 450.0f;
	FVector TraceEnd = TraceStart + FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::X) * TraceDistance;
	
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(GetOwningActorFromActorInfo());

	// Trace from camera to find the target location
	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), TraceStart, TraceEnd, UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1), false, IgnoredActors, EDrawDebugTrace::None, HitResult, true);

	// If hit, set the target location to the hit location, otherwise set it to the end of the trace
	FVector TargetLocation;
	if (bHit)
	{
		// If trace target is too close, simply use the camera rotation
		if (HitResult.Distance < 45.f)
		{
			return ArenaPC->GetControlRotation();
		}

		// Trace target is within prediction distance, use the hit location to predict the launch rotation
		if (HitResult.Distance < PredictionDistance)
		{
			TargetLocation = HitResult.Location;
		}

		// Trace target is far away, use the end of the trace to predict so that aiming feels consistent
		else
		{
			TargetLocation = TraceEnd;
		}
	}
	else
	{
		TargetLocation = TraceEnd;
	}

	const float LaunchSpeed = GrenadeDefinitionData->ProjectileSpeed;
	const float GravityScale = GrenadeDefinitionData->GravityScale;
	return CalculateLaunchRotation(GetWorld(), GetSpawnLocation(), TargetLocation, LaunchSpeed, GravityScale);
}

// ChatGPT o3-mini-high wrote the following code snippet. Amazing job!
FRotator UArenaGameplayAbility_Grenade::CalculateLaunchRotation(const UWorld* World, const FVector& Start, const FVector& Target, const float LaunchSpeed, const float GravityScale)
{
	// // Draw a start and target as debug spheres
	// DrawDebugSphere(World, Start, 5.f, 12, FColor::Green, false, 0.1f);
	// DrawDebugSphere(World, Target, 5.f, 12, FColor::Red, false, 0.1f);
	//
	// // draw a line between start and target
	// DrawDebugLine(World, Start, Target, FColor::Blue, false, 0.1f);
	
	// Compute the difference vector between Target and Start.
	FVector Diff = Target - Start;
    
	// Separate horizontal (XY) component and compute its length.
	FVector DiffXY = FVector(Diff.X, Diff.Y, 0.f);
	float d = DiffXY.Size();
    
	// Vertical difference.
	float dz = Diff.Z;
    
	// If the horizontal distance is nearly zero, fire straight up or down.
	if (d < KINDA_SMALL_NUMBER)
	{
		return FRotator((dz >= 0.f) ? 90.f : -90.f, 0.f, 0.f);
	}
    
	// Get effective gravity.
	// Note: GetGravityZ() returns a negative value in Unreal Engine,
	// so we take its absolute value and multiply by GravityScale.
	float g = FMath::Abs(World->GetGravityZ()) * GravityScale;
    
	// If gravity is zero (or nearly zero), simply shoot directly towards the target.
	if (FMath::IsNearlyZero(g))
	{
		return Diff.GetSafeNormal().Rotation();
	}
    
	// Precompute squared launch speed.
	float v2 = FMath::Square(LaunchSpeed);
    
	// Compute the discriminant for the projectile equation.
	float Discriminant = v2 * v2 - g * (g * d * d + 2.f * dz * v2);
    
	float Theta; // The launch angle in radians.
	if (Discriminant < 0.f)
	{
		// The target is unreachable with the given LaunchSpeed. Fall back to a default angle.
		// For level ground this gives 45 degrees, which is the maximum-range angle.
		// But when 45 degrees looks weird in tracing trajectory, we can use 30 degrees.
		// The player need to aim higher for maximum range.
		Theta = FMath::DegreesToRadians(30.f);
	}
	else
	{
		// Use the low-angle (direct shot) solution.
		Theta = FMath::Atan((v2 - FMath::Sqrt(Discriminant)) / (g * d));
	}
    
	// Construct the launch direction vector.
	// It has a horizontal component in the direction of DiffXY and a vertical component.
	FVector LaunchDirection = DiffXY.GetSafeNormal() * FMath::Cos(Theta) + FVector::UpVector * FMath::Sin(Theta);

	// // Draw the launch direction as a debug line.
	// DrawDebugLine(World, Start, Start + LaunchDirection * 1000.f, FColor::Yellow, false, 0.1f);
	
	// Return the rotation corresponding to this direction.
	return LaunchDirection.Rotation();
}

