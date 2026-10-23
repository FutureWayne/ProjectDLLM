// Copyright Ludens Studio. All Rights Reserved.

#include "Character/ArenaMeshAssemblyComponent.h"

#include "Character/ArenaHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "TimerManager.h"

UArenaMeshAssemblyComponent::UArenaMeshAssemblyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UArenaMeshAssemblyComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoPlay && GetNetMode() != NM_DedicatedServer)
	{
		// Characters start synchronously after Blueprint BeginPlay; other owners retain this fallback.
		AutoPlayTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::AutoPlayAssembly);
	}
}

void UArenaMeshAssemblyComponent::AutoPlayAssembly()
{
	if (bAutoPlay && !IsAssembling())
	{
		PlayAssembly();
	}
}

void UArenaMeshAssemblyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAssembly();
	Super::EndPlay(EndPlayReason);
}

void UArenaMeshAssemblyComponent::PlayAssembly(USkeletalMeshComponent* TargetMesh)
{
	StopAssembly();
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || Owner->IsHidden() || GetNetMode() == NM_DedicatedServer || !AssemblySystem ||
		!FMath::IsFinite(Duration) || Duration < 0.1f)
	{
		return;
	}
	if (const UArenaHealthComponent* Health = Owner->FindComponentByClass<UArenaHealthComponent>(); Health && Health->IsDeadOrDying())
	{
		return;
	}

	TArray<USkeletalMeshComponent*> Meshes;
	if (TargetMesh)
	{
		if (TargetMesh->GetOwner() != Owner)
		{
			return;
		}
		Meshes.Add(TargetMesh);
	}
	else
	{
		Owner->GetComponents(Meshes);
	}

	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh) || !Mesh->GetSkeletalMeshAsset() || !Mesh->IsVisible() || Mesh->bHiddenInGame)
		{
			continue;
		}

		// Require variants for every slot so an unsupported mesh never appears half assembled.
		TArray<UMaterialInterface*> Variants;
		for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
		{
			UMaterialInterface* Source = Mesh->GetMaterial(Index);
			while (const UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Source))
			{
				Source = Dynamic->Parent;
			}
			if (!Source)
			{
				Source = UMaterial::GetDefaultMaterial(MD_Surface);
			}
			const TObjectPtr<UMaterialInterface>* Variant = ScanMaterials.Find(Source);
			if (!Variant || !*Variant)
			{
				break;
			}
			Variants.Add(*Variant);
		}
		if (Variants.IsEmpty() || Variants.Num() != Mesh->GetNumMaterials())
		{
			continue;
		}

		UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(AssemblySystem, Mesh, NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, false,
			ENCPoolMethod::None, false);
		if (!Effect)
		{
			continue;
		}
		Effects.Add(Effect);
		Effect->AddTickPrerequisiteComponent(Mesh);
		for (int32 Index = 0; Index < Variants.Num(); ++Index)
		{
			FArenaAssemblyMaterialSlot& Slot = MaterialSlots.AddDefaulted_GetRef();
			Slot.Mesh = Mesh;
			Slot.Index = Index;
			Slot.Original = Mesh->GetMaterial(Index);
			Slot.Assembly = UMaterialInstanceDynamic::Create(Variants[Index], this);
			Slot.Assembly->CopyMaterialUniformParameters(Slot.Original ? Slot.Original.Get() : UMaterial::GetDefaultMaterial(MD_Surface));
			Mesh->SetMaterial(Index, Slot.Assembly);
		}
	}

	Elapsed = 0.0f;
	UpdatePresentation(0.0f);
	for (UNiagaraComponent* Effect : Effects)
	{
		Effect->Activate(true);
	}
	SetComponentTickEnabled(IsAssembling());
}

void UArenaMeshAssemblyComponent::UpdatePresentation(float Progress)
{
	FLinearColor Color(0.025f, 0.65f, 1.0f);
	if (const UArenaTeamSubsystem* Teams = GetWorld()->GetSubsystem<UArenaTeamSubsystem>())
	{
		if (Teams->FindTeamFromObject(GetOwner()) != INDEX_NONE)
		{
			Color = Teams->FindTeamColorFromActor(GetOwner());
		}
	}

	for (const FArenaAssemblyMaterialSlot& Slot : MaterialSlots)
	{
		if (USkeletalMeshComponent* Mesh = Slot.Mesh.Get(); Mesh && Mesh->GetMaterial(Slot.Index) == Slot.Assembly)
		{
			const FBoxSphereBounds Bounds = Mesh->Bounds;
			Slot.Assembly->SetScalarParameterValue(TEXT("AssemblyProgress"), Progress);
			Slot.Assembly->SetScalarParameterValue(TEXT("AssemblyMinZ"), Bounds.Origin.Z - Bounds.BoxExtent.Z);
			Slot.Assembly->SetScalarParameterValue(TEXT("AssemblyHeight"), FMath::Max(2.0 * Bounds.BoxExtent.Z, 1.0));
			Slot.Assembly->SetVectorParameterValue(TEXT("AssemblyColor"), PrintingColor);
		}
	}
	for (UNiagaraComponent* Effect : Effects)
	{
		if (IsValid(Effect))
		{
			const USkeletalMeshComponent* Mesh = CastChecked<USkeletalMeshComponent>(Effect->GetAttachParent());
			const FBoxSphereBounds Bounds = Mesh->Bounds;
			Effect->SetVariableFloat(TEXT("User.AssemblyProgress"), Progress);
			Effect->SetVariableFloat(TEXT("User.AssemblyMinZ"), Bounds.Origin.Z - Bounds.BoxExtent.Z - Mesh->GetComponentLocation().Z);
			Effect->SetVariableFloat(TEXT("User.AssemblyHeight"), FMath::Max(2.0 * Bounds.BoxExtent.Z, 1.0));
			Effect->SetVariableVec3(TEXT("User.BodyColor"), FVector(Color.R, Color.G, Color.B));
			Effect->SetVariableVec3(TEXT("User.PrintColor"), FVector(PrintingColor.R, PrintingColor.G, PrintingColor.B));
			Effect->SetVariableFloat(TEXT("User.AssemblyRadius"), FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y));
			Effect->SetVariableVec3(TEXT("User.AssemblyCenter"), Bounds.Origin - Mesh->GetComponentLocation());
		}
	}
}

void UArenaMeshAssemblyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UArenaHealthComponent* Health = GetOwner()->FindComponentByClass<UArenaHealthComponent>();
	if (GetOwner()->IsHidden() || (Health && Health->IsDeadOrDying()) || !FMath::IsFinite(Duration) || Duration < 0.1f)
	{
		StopAssembly();
		return;
	}
	Elapsed += FMath::Max(DeltaTime, 0.0f);
	const float Progress = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
	UpdatePresentation(Progress);
	if (Progress >= 1.0f)
	{
		StopAssembly();
	}
}

void UArenaMeshAssemblyComponent::StopAssembly()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoPlayTimer);
	}
	for (const FArenaAssemblyMaterialSlot& Slot : MaterialSlots)
	{
		// A death cue or equipment change may have replaced our material in the meantime.
		if (USkeletalMeshComponent* Mesh = Slot.Mesh.Get(); Mesh && Mesh->GetMaterial(Slot.Index) == Slot.Assembly)
		{
			Mesh->SetMaterial(Slot.Index, Slot.Original);
		}
	}
	MaterialSlots.Reset();
	for (UNiagaraComponent* Effect : Effects)
	{
		if (IsValid(Effect))
		{
			Effect->DestroyComponent();
		}
	}
	Effects.Reset();
	SetComponentTickEnabled(false);
}
