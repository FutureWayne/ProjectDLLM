// Copyright Ludens Studio. All Rights Reserved.


#include "Equipment/ArenaEquipmentInstance.h"

#include "NativeGameplayTags.h"
#include "Character/DEPRECATED_ABlasterCharacter.h"
#include "Equipment/ArenaEquipmentDefinition.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaEquipmentInstance)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Arena_Equipment_Message_EquipmentChanged, "Arena.Equipment.Message.EquipmentChanged");

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

void UArenaEquipmentInstance::SetInstigator(UObject* InInstigator)
{
	Instigator = InInstigator;

	FArenaEquipmentChangedMessage Message;
	Message.Owner = GetPawn();
	Message.EquipmentInstance = this;
	
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetPawn());
	MessageSystem.BroadcastMessage(TAG_Arena_Equipment_Message_EquipmentChanged, Message);
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
		if (AArenaCharacter* Char = Cast<AArenaCharacter>(OwningPawn))
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
	FArenaEquipmentChangedMessage Message;
	Message.Owner = GetPawn();
	Message.EquipmentInstance = this;
	
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetPawn());
	MessageSystem.BroadcastMessage(TAG_Arena_Equipment_Message_EquipmentChanged, Message);
}
