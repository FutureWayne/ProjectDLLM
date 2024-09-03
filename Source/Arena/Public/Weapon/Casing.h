// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casing.generated.h"

class USoundCue;

UCLASS()
class ARENA_API ACasing : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasing();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnywhere, Category = "Casing")
	TObjectPtr<UStaticMeshComponent> CasingMesh;

	UPROPERTY(EditAnywhere, Category = "Casing")
	float ShellEjectImpulse;

	UPROPERTY(EditAnywhere, Category = "Casing")
	TObjectPtr<USoundCue> ShellSound;
};
