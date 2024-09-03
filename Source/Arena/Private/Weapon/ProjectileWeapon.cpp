// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (!HasAuthority())
	{
		return;
	}

	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	if (const USkeletalMeshSocket* MuzzleSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash")))
	{
		FTransform MuzzleTransform = MuzzleSocket->GetSocketTransform(GetWeaponMesh());
		FVector ToTarget = HitTarget - MuzzleTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();
		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Instigator = InstigatorPawn;
			SpawnParams.Owner = GetOwner();

			if (UWorld* World = GetWorld())
			{
				World->SpawnActor<AProjectile>(
					ProjectileClass,
					MuzzleTransform.GetLocation(),
					TargetRotation,
					SpawnParams);
			}
		}
	}
}
