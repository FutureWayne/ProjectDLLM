// Copyright Ludens Studio. All Rights Reserved.

#pragma once

#include "Camera/PlayerCameraManager.h"

#include "ArenaPlayerCameraManager.generated.h"

class FDebugDisplayInfo;
class UCanvas;
class UObject;


#define ARENA_CAMERA_DEFAULT_FOV		(80.0f)
#define ARENA_CAMERA_DEFAULT_PITCH_MIN	(-89.0f)
#define ARENA_CAMERA_DEFAULT_PITCH_MAX	(89.0f)

class UArenaUICameraManagerComponent;

/**
 * AArenaPlayerCameraManager
 *
 *	The base player camera manager class used by this project.
 */
UCLASS(notplaceable, Blueprintable)
class ARENA_API AArenaPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	AArenaPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	UArenaUICameraManagerComponent* GetUICameraComponent() const;

	/** Keeps the victim camera and turns its view toward the original killer. */
	void StartKillCamera(APawn* Victim, APawn* Killer);
	void StopKillCamera();
	bool IsKillCameraActive() const { return bKillCameraActive; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Blueprint owns stencil/material presentation, including restoring the previous target. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|KillCamera")
	void UpdateKillCameraHighlight(APawn* Killer, bool bEnabled);

	/** Duration of the initial look-at rotation, in game seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|KillCamera", meta = (ClampMin = "0.01"))
	float KillCameraBlendDuration = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|KillCamera", meta = (ClampMin = "0.0"))
	float KillCameraFollowSmoothingSpeed = 10.0f;

	/** World-space offset applied once to the victim's normal camera position. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|KillCamera")
	FVector KillCameraAnchorOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|KillCamera")
	FPostProcessSettings KillCameraPostProcessSettings;


	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

private:
	void UpdateKillCamera(FTViewTarget& OutVT, float DeltaTime);

	TWeakObjectPtr<APawn> KillCameraVictim;
	TWeakObjectPtr<APawn> KillCameraKiller;
	FMinimalViewInfo LastVictimCameraView;
	FVector KillCameraAnchorLocation = FVector::ZeroVector;
	FRotator KillCameraBlendStartRotation = FRotator::ZeroRotator;
	FRotator LastKillCameraRotation = FRotator::ZeroRotator;
	float KillCameraBlendElapsed = 0.0f;
	bool bKillCameraActive = false;

	/** The UI Camera Component, controls the camera when UI is doing something important that gameplay doesn't get priority over. */
	UPROPERTY(Transient)
	TObjectPtr<UArenaUICameraManagerComponent> UICamera;
};

