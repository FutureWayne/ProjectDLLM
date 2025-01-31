// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/DEPRECATED_AArenaMinion.h"
#include <BehaviorTree/BTNode.h>
#include "DEPRECATED_AAIArenaMinionCharacter.generated.h"

/**
 * 
 */
UCLASS(Deprecated)
class ARENA_API ADEPRECATED_AAIArenaMinionCharacter : public ADEPRECATED_AArenaMinion
{
	GENERATED_BODY()
public:
	ADEPRECATED_AAIArenaMinionCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectRangeRadius = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	AActor* TowerTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;
	
	virtual void OnDeathStarted(AActor* OwningActor) override;
	
	virtual void OnDeathFinished(AActor* OwningActor) override;

	UFUNCTION(BlueprintCallable, Category = "AIArenaMinion")
	void SetTowerTarget(AActor* Target);

protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;
};
