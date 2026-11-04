// Copyright Ludens Studio. All Rights Reserved.

#include "Camera/ArenaPlayerCameraManager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "Character/ArenaHealthComponent.h"
#include "Character/ArenaCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/ArenaCameraComponent.h"
#include "Camera/ArenaUICameraManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ArenaPlayerCameraManager)

class FDebugDisplayInfo;

static FName UICameraComponentName(TEXT("UICamera"));

AArenaPlayerCameraManager::AArenaPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = ARENA_CAMERA_DEFAULT_FOV;
	ViewPitchMin = ARENA_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = ARENA_CAMERA_DEFAULT_PITCH_MAX;

	UICamera = CreateDefaultSubobject<UArenaUICameraManagerComponent>(UICameraComponentName);
}

UArenaUICameraManagerComponent* AArenaPlayerCameraManager::GetUICameraComponent() const
{
	return UICamera;
}

void AArenaPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	if (bKillCameraActive)
	{
		APawn* CurrentPawn = PCOwner ? PCOwner->GetPawn() : nullptr;
		const UArenaHealthComponent* Health = UArenaHealthComponent::FindHealthComponent(CurrentPawn);
		if (CurrentPawn && CurrentPawn != KillCameraVictim.Get() && (!Health || !Health->IsDeadOrDying()))
		{
			StopKillCamera();
		}
	}

	// If the UI Camera is looking at something, let it have priority.
	if (UICamera->NeedsToUpdateViewTarget())
	{
		Super::UpdateViewTarget(OutVT, DeltaTime);
		UICamera->UpdateViewTarget(OutVT, DeltaTime);
		return;
	}

	if (bKillCameraActive)
	{
		if (APawn* Victim = KillCameraVictim.Get())
		{
			// The authority view target can remain the killer for network relevancy.
			// Evaluate the victim's original camera so its FOV blend and modifiers keep running.
			FTViewTarget VictimViewTarget = OutVT;
			VictimViewTarget.Target = Victim;
			Super::UpdateViewTarget(VictimViewTarget, DeltaTime);
			LastVictimCameraView = VictimViewTarget.POV;
		}
		OutVT.POV = LastVictimCameraView;
		OutVT.POV.Location = KillCameraAnchorLocation;
		UpdateKillCamera(OutVT, DeltaTime);
		return;
	}

	Super::UpdateViewTarget(OutVT, DeltaTime);
}

void AArenaPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("ArenaPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	if (const UArenaCameraComponent* CameraComponent = UArenaCameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DrawDebug(Canvas);
	}
}


void AArenaPlayerCameraManager::StartKillCamera(APawn* Victim, APawn* Killer)
{
	StopKillCamera();
	// Suicide and environmental deaths retain the existing death camera and disappearance sequence.
	if (!IsValid(Victim) || !IsValid(Killer) || Killer == Victim || !PCOwner || !PCOwner->IsLocalController())
	{
		return;
	}

	KillCameraVictim = Victim;
	KillCameraKiller = Killer;
	LastVictimCameraView = GetCameraCacheView();
	KillCameraAnchorLocation = LastVictimCameraView.Location + KillCameraAnchorOffset;
	KillCameraBlendStartRotation = LastVictimCameraView.Rotation;
	LastKillCameraRotation = KillCameraBlendStartRotation;
	KillCameraBlendElapsed = 0.0f;
	bKillCameraActive = true;
	UpdateKillCameraHighlight(Killer, true);
}

void AArenaPlayerCameraManager::StopKillCamera()
{
	if (bKillCameraActive && KillCameraKiller.IsValid())
	{
		UpdateKillCameraHighlight(KillCameraKiller.Get(), false);
	}
	bKillCameraActive = false;
	KillCameraVictim.Reset();
	KillCameraKiller.Reset();
}

void AArenaPlayerCameraManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopKillCamera();
	Super::EndPlay(EndPlayReason);
}

void AArenaPlayerCameraManager::UpdateKillCamera(FTViewTarget& OutVT, float DeltaTime)
{
	if (APawn* Killer = KillCameraKiller.Get())
	{
		FVector AimLocation = Killer->GetActorLocation();
		if (AArenaCharacter* KillerCharacter = Cast<AArenaCharacter>(Killer))
		{
			if (const USkeletalMeshComponent* DisplayMesh = KillerCharacter->GetDisplayMesh())
			{
				AimLocation = DisplayMesh->Bounds.Origin;
			}
		}
		const FVector LookDirection = AimLocation - OutVT.POV.Location;
		if (!LookDirection.IsNearlyZero())
		{
			FRotator DesiredRotation = LookDirection.Rotation();
			DesiredRotation.Roll = LastVictimCameraView.Rotation.Roll;
			KillCameraBlendElapsed += FMath::Max(DeltaTime, 0.0f);
			const float BlendFraction = FMath::Clamp(KillCameraBlendElapsed / FMath::Max(KillCameraBlendDuration, 0.01f), 0.0f, 1.0f);
			if (BlendFraction < 1.0f)
			{
				const float BlendWeight = FMath::SmoothStep(0.0f, 1.0f, BlendFraction);
				LastKillCameraRotation = FQuat::Slerp(KillCameraBlendStartRotation.Quaternion(),
					DesiredRotation.Quaternion(), BlendWeight).Rotator();
			}
			else
			{
				LastKillCameraRotation = FMath::RInterpTo(LastKillCameraRotation, DesiredRotation,
					DeltaTime, KillCameraFollowSmoothingSpeed);
			}
		}
		// Camera updates run after pawn ticks, so the normal team stencil cannot erase this highlight.
		UpdateKillCameraHighlight(Killer, true);
	}

	// Never feed the look-at rotation back into the victim's control rotation or orbit position.
	// Once the corpse expires, keep its last camera position and FOV until the owner respawns.
	OutVT.POV.Rotation = LastKillCameraRotation;
	OutVT.POV.PostProcessSettings = KillCameraPostProcessSettings;
	OutVT.POV.PostProcessBlendWeight = 1.0f;
}
