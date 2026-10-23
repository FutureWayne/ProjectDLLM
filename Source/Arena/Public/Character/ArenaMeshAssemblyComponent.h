// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "ArenaMeshAssemblyComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class USkeletalMeshComponent;

USTRUCT()
struct FArenaAssemblyMaterialSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> Original;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Assembly;

	int32 Index = INDEX_NONE;
};

/** Cosmetic reverse-deconstruction shared by respawning characters and summoned creatures. */
UCLASS(ClassGroup = (Arena), meta = (BlueprintSpawnableComponent))
class ARENA_API UArenaMeshAssemblyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArenaMeshAssemblyComponent();

	/** Runs locally on each rendered instance; gameplay movement and collision continue normally. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Assembly")
	bool bAutoPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Assembly")
	TObjectPtr<UNiagaraSystem> AssemblySystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Assembly", meta = (ClampMin = "0.1", Units = "s"))
	float Duration = 0.55f;

	/** Laser and hot print-edge color, independent of the model's team-colored voxels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Assembly")
	FLinearColor PrintingColor = FLinearColor(1.0f, 0.035f, 0.006f);

	/** Original material (or a dynamic instance's parent) -> scan variant preserving its surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Assembly")
	TMap<TObjectPtr<UMaterialInterface>, TObjectPtr<UMaterialInterface>> ScanMaterials;

	/** An explicit mesh supports summoned actors; null selects the owner's visible skeletal meshes. */
	UFUNCTION(BlueprintCallable, Category = "Arena|Assembly")
	void PlayAssembly(USkeletalMeshComponent* TargetMesh = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Arena|Assembly")
	void StopAssembly();

	UFUNCTION(BlueprintPure, Category = "Arena|Assembly")
	bool IsAssembling() const { return !MaterialSlots.IsEmpty(); }

	/** Starts once after the owner has installed its display mesh and materials in Blueprint BeginPlay. */
	void AutoPlayAssembly();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void UpdatePresentation(float Progress);

	UPROPERTY(Transient)
	TArray<FArenaAssemblyMaterialSlot> MaterialSlots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> Effects;

	FTimerHandle AutoPlayTimer;
	float Elapsed = 0.0f;
};
