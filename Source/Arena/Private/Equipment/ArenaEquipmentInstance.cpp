// Copyright Ludens Studio. All Rights Reserved.


#include "Equipment/ArenaEquipmentInstance.h"

#include "Character/BlasterCharacter.h"
#include "Equipment/ArenaEquipmentDefinition.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaEquipmentInstance)

UArenaEquipmentInstance::UArenaEquipmentInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UWorld* UArenaEquipmentInstance::GetWorld() const
{
	if (APawn* OwningPawn = GetPawn())
	{
		return OwningPawn->GetWorld();
	}
	else
	{
		return nullptr;
	}
}

void UArenaEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Instigator);
	DOREPLIFETIME(ThisClass, SpawnedActors);
}

APawn* UArenaEquipmentInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

APawn* UArenaEquipmentInstance::GetTypedPawn(const TSubclassOf<APawn>& PawnType) const
{
	APawn* Result = nullptr;
	if (UClass* ActualPawnType = PawnType)
	{
		if (GetOuter()->IsA(ActualPawnType))
		{
			Result = Cast<APawn>(GetOuter());
		}
	}

	return Result;
}

void UArenaEquipmentInstance::SpawnEquipmentActors(const TArray<FArenaEquipmentActorToSpawn>& ActorsToSpawn)
{
	if (APawn* OwningPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
		if (ABlasterCharacter* Char = Cast<ABlasterCharacter>(OwningPawn))
		{
			AttachTarget = Char->GetDisplayMesh();
		}

		for (const FArenaEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
		{
			AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, OwningPawn);
			NewActor->FinishSpawning(FTransform::Identity, true);
			NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);

			SpawnedActors.Add(NewActor);
		}
		
	}
}

void UArenaEquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
}

void UArenaEquipmentInstance::OnEquipped()
{
	K2_OnEquipped();
}

void UArenaEquipmentInstance::OnUnequipped()
{
	K2_OnUnequipped();
}

void UArenaEquipmentInstance::OnRep_Instigator()
{
}
