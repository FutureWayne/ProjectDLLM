#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

class AArenaCharacter;

#include "ArenaExternalRadialImpactMessage.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_EXTERNAL_RADIAL_IMPACT_MESSAGE);

USTRUCT(BlueprintType)
struct FArenaExternalRadialImpactMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AArenaCharacter> AffectedCharacter = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	float Force = 0.0f;
};

