// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ArenaMinion.h"
#include <BehaviorTree/BTNode.h>
#include "AIArenaMinionCharacter.generated.h"

/**
 * 
 */
UCLASS()
class ARENA_API AAIArenaMinionCharacter : public AArenaMinion
{
	GENERATED_BODY()
public:
	AAIArenaMinionCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectRangeRadius = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	AActor* TowerTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	UFUNCTION(BlueprintCallable, Category = "AIArenaMinion")
	void SetTowerTarget(AActor* Target);

protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;
};
