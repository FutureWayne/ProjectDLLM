// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaGameplayTags.h"
#include "ArenaDeploymentDefinitionData.generated.h"

ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_DeployPolicy_Actor);
ARENA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_DeployPolicy_Character);

class UArenaGrenadeDefinitionData;

UCLASS()
class ARENA_API UArenaDeploymentDefinitionData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	FVector DeployLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	FRotator DeployRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	FVector DeployScaleOffset = FVector::OneVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	TSubclassOf<AActor> DeployActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	TSubclassOf<AController> DeployActorController = nullptr;
	
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
