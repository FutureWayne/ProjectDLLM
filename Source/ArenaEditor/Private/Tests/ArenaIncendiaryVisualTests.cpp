// Copyright Ludens Studio. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Actor/ArenaGroundAreaEffect.h"
#include "Actor/ArenaGroundAreaDefinition.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"
#include "Weapon/ArenaGrenadeBase.h"
#include "System/ArenaSystemStatics.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "HAL/FileManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PlayInEditorDataTypes.h"
#include "RenderingThread.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "TextureResource.h"
#include "UObject/UObjectIterator.h"

namespace
{
	struct FGroundFireCaptureFixture
	{
		FSoftClassPath SavedGameInstance = GetDefault<UGameMapsSettings>()->GameInstanceClass;

		~FGroundFireCaptureFixture()
		{
			GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = SavedGameInstance;
		}
	};

	class FCaptureGroundFire : public IAutomationLatentCommand
	{
	public:
		FCaptureGroundFire(FAutomationTestBase* InTest, TSharedRef<FGroundFireCaptureFixture> InFixture)
			: Test(InTest)
			, Fixture(InFixture)
		{
		}

		virtual bool Update() override
		{
			if (GetCurrentRunTime() > 45.)
			{
				Test->AddError(TEXT("Ground fire PIE capture timed out"));
				return true;
			}

			UWorld* World = GEditor->PlayWorld;
			if (!World || !World->HasBegunPlay())
			{
				return false;
			}

			if (!bSpawned)
			{
				const UArenaGrenadeDefinitionData* Grenade = LoadObject<UArenaGrenadeDefinitionData>(nullptr,
					TEXT("/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade/DA_IncendiaryGrenadeDef.DA_IncendiaryGrenadeDef"));
				if (!Test->TestNotNull(TEXT("Production grenade loads for render test"), Grenade))
				{
					return true;
				}

				if (!Test->TestTrue(TEXT("Production area is configured"), Grenade->EffectActorsToSpawn.Num() > 0))
				{
					return true;
				}

				const UArenaGroundAreaDefinition* Area = Cast<UArenaGroundAreaDefinition>(Grenade->EffectActorsToSpawn[0].EffectDefinition);
				if (!Test->TestNotNull(TEXT("Production ground-area definition loads"), Area))
				{
					return true;
				}

				const FTransform Transform(FVector(137, -93, 1500));
				AArenaGrenadeBase* Projectile = UArenaSystemStatics::SpawnGrenadeByGrenadeDefinition(World, Transform, Grenade, nullptr, nullptr);
				if (!Test->TestNotNull(TEXT("Production grenade spawns for full visual sequence"), Projectile))
				{
					return true;
				}

				UProjectileMovementComponent* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
				Movement->StopMovementImmediately();
				Movement->ProjectileGravityScale = 0.f;
				Projectile->SetGrenadeExplosionTimer(0.1f);
				bSpawned = true;
				StartedAt = World->GetTimeSeconds();
				AActor* Actor = World->SpawnActor<AActor>();
				USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(Actor);
				Actor->SetRootComponent(Capture);
				Capture->bCaptureEveryFrame = true;
				Capture->bCaptureOnMovement = true;
				Capture->bAlwaysPersistRenderingState = true;
				Capture->FOVAngle = 65.f;
				Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
				Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
				Capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
				Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
				Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
				Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
				Capture->PostProcessSettings.AutoExposureBias = 2.f;
				Capture->PostProcessSettings.bOverride_BloomIntensity = true;
				Capture->PostProcessSettings.BloomIntensity = 0.3f;
				UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(Actor);
				Target->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
				Target->InitAutoFormat(1024, 768);
				Capture->TextureTarget = Target;
				const FVector ViewCenter = Transform.GetLocation();
				const FVector Camera = ViewCenter + FVector(650, -650, 800);
				Actor->SetActorLocationAndRotation(Camera, (ViewCenter - Camera).Rotation());
				Capture->RegisterComponent();
				RenderTarget = Target;
				CameraActor = Actor;
				AimAt(ViewCenter);
				return false;
			}

			if (!Field.IsValid())
			{
				for (TActorIterator<AArenaGroundAreaEffect> It(World); It; ++It)
				{
					Field = *It;
				}

				if (!Field.IsValid())
				{
					return false;
				}
			}

			// Allow the first Niagara simulation/render frames after a cold editor load.
			if (!bBurstCaptured && World->GetTimeSeconds() - StartedAt > 0.6f)
			{
				int32 Bursts = 0;
				for (TObjectIterator<UNiagaraComponent> It; It; ++It)
				{
					if (It->GetWorld() == World && It->GetAsset() && It->GetAsset()->GetName() == TEXT("NS_Grenade_Explosion"))
					{
						++Bursts;
						Test->AddInfo(FString::Printf(TEXT("Explosion Niagara active=%d complete=%d position=%s"),
							It->IsActive(), It->IsComplete(), *It->GetComponentLocation().ToString()));
						Test->TestTrue(TEXT("Explosion stays at its airborne detonation point"),
							It->GetComponentLocation().Equals(FVector(137, -93, 1500), 1.f));
					}
				}

				Test->TestTrue(TEXT("Grenade executes its own explosion Niagara at altitude"), Bursts > 0);
				SaveImage(World, TEXT("Airburst.png"));
				bBurstCaptured = true;
				// Isolate the next capture: lingering explosion sparks must not satisfy the falling-fire pixel check.
				for (TObjectIterator<UNiagaraComponent> It; It; ++It)
				{
					if (It->GetWorld() == World && It->GetAsset() && It->GetAsset()->GetName() == TEXT("NS_Grenade_Explosion"))
					{
						It->SetVisibility(false, true);
					}
				}
			}

			if (Field->IsFalling())
			{
				AimAt(Field->GetActorLocation());
				if (!bFallingCaptured && World->GetTimeSeconds() - StartedAt > 0.9f)
				{
					UNiagaraComponent* Visual = Field->GetFallingVisual();
					AirborneVisual = Visual;
					Test->AddInfo(TEXT("Falling fire position: ") + Field->GetActorLocation().ToString());
					Test->TestTrue(TEXT("Rendered falling fire actually descends below the explosion"),
						Field->GetActorLocation().Z > 2.f && Field->GetActorLocation().Z < 1400.f);
					Test->TestTrue(TEXT("Falling fire Niagara stays active and attached to its descending field"),
						IsValid(Visual) && Visual->IsActive() && !Visual->IsComplete() && Visual->GetComponentLocation().Equals(Field->GetActorLocation(), 1.f));
					Test->TestTrue(TEXT("Falling visual does not create ground display cells"), Field->GetVisualCells().IsEmpty());
					SaveImage(World, TEXT("FallingFire.png"));
					bFallingCaptured = true;
				}

				return false;
			}

			if (LandedAt < 0.)
			{
				Test->TestTrue(TEXT("The rendered sequence includes descent before ground fire"), bFallingCaptured);
				Test->TestNull(TEXT("Landing releases the airborne emitter"), Field->GetFallingVisual());
				Test->TestTrue(TEXT("Existing airborne particles survive ground contact"), AirborneVisual.IsValid() && !AirborneVisual->IsComplete());
				if (AirborneVisual.IsValid())
				{
					Test->TestNull(TEXT("Residual particles detach from the grounded field"), AirborneVisual->GetAttachParent());
				}
				LandedAt = World->GetTimeSeconds();
				AimAt(Field->GetActorLocation());
			}

			if (!bLingeringCaptured && World->GetTimeSeconds() - LandedAt > 0.35f)
			{
				Test->TestTrue(TEXT("Airborne residuals remain alive while ground fire spreads"), AirborneVisual.IsValid() && !AirborneVisual->IsComplete());
				SaveImage(World, TEXT("LingeringFire.png"));
				bLingeringCaptured = true;
			}

			if (World->GetTimeSeconds() - LandedAt < 2.4f || !Field->IsDiffusionComplete() ||
				Field->GetSpawnedVisualCount() < Field->GetVisualCells().Num())
			{
				return false;
			}

			int32 Active = 0;
			Test->TestTrue(TEXT("Airborne residuals finish and clean up after their lifetime"), !AirborneVisual.IsValid() || AirborneVisual->IsComplete());
			int32 BoundaryMaterials = 0;
			for (TObjectIterator<UNiagaraComponent> It; It; ++It)
			{
				if (It->GetWorld() == World && It->GetAsset() == Field->GetSettings().CellEffect)
				{
					Test->AddInfo(FString::Printf(TEXT("Ground flame active=%d complete=%d"), It->IsActive(), It->IsComplete()));
					if (It->IsActive() && !It->IsComplete())
					{
						++Active;
					}

					const FNiagaraVariable Parameter(FNiagaraTypeDefinition::GetUMaterialDef(), TEXT("User.GroundAreaMaterial"));
					if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(It->GetOverrideParameters().GetUObject(Parameter)))
					{
						++BoundaryMaterials;
						const FLinearColor Center = Material->K2_GetVectorParameterValue(TEXT("GroundAreaCenter"));
						Test->TestTrue(TEXT("Boundary follows the area at a nonzero world position"),
							FMath::IsNearlyEqual(Center.R, Field->GetActorLocation().X) && FMath::IsNearlyEqual(Center.G, Field->GetActorLocation().Y));
						Test->TestEqual(TEXT("Boundary uses the entire area's radius"), Material->K2_GetScalarParameterValue(TEXT("GroundAreaRadius")), Field->GetSettings().Radius);
						Test->TestEqual(TEXT("Organic boundary is enabled"), Material->K2_GetScalarParameterValue(TEXT("GroundAreaMaskEnabled")), 1.f);
					}
				}
			}

			Test->TestTrue(TEXT("Production Niagara systems remain active after two real seconds"), Active > 0);
			Test->TestEqual(TEXT("Every display cell shares the whole-area material mask"), BoundaryMaterials, Field->GetSpawnedVisualCount());
			Test->TestEqual(TEXT("Every display cell has a Niagara component"), Field->GetSpawnedVisualCount(), Field->GetVisualCells().Num());
			SaveImage(World, TEXT("GroundFire.png"));
			return true;
		}

	private:
		void AimAt(FVector Center)
		{
			const bool bFixedView = FParse::Param(FCommandLine::Get(), TEXT("DriftFuelTrailCapture"));
			const FVector Camera = bFixedView ? FVector(1100, -1200, 2100) : Center + FVector(650, -650, 800);
			if (bFixedView)
			{
				Center = FVector(137, -93, 900);
				CastChecked<USceneCaptureComponent2D>(CameraActor->GetRootComponent())->FOVAngle = 80.f;
			}

			CameraActor->SetActorLocationAndRotation(Camera, (Center - Camera).Rotation());
		}

		void SaveImage(UWorld* World, const FString& Name)
		{
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
			int32 FlamePixels = 0;
			for (const FColor Pixel : Pixels)
			{
				FlamePixels += Pixel.R > 80 && Pixel.R > Pixel.G * 1.12f && Pixel.G > Pixel.B * 1.2f;
			}

			Test->TestTrue(TEXT("Sequence stage produces visible warm flame pixels: ") + Name, FlamePixels > 1000);
			Test->AddInfo(FString::Printf(TEXT("%s visible flame pixels: %d"), *Name, FlamePixels));
			const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/IncendiaryVisual");
			IFileManager::Get().MakeDirectory(*Directory, true);
			const FString Filename = Directory / Name;
			IFileManager::Get().Delete(*Filename);
			UKismetRenderingLibrary::ExportRenderTarget(World, RenderTarget.Get(), Directory, Name);
			Test->TestTrue(TEXT("Rendered ground-fire preview is written"), IFileManager::Get().FileExists(*Filename));
			Test->AddInfo(TEXT("Rendered preview: ") + Filename);
		}

		FAutomationTestBase* Test;
		TSharedRef<FGroundFireCaptureFixture> Fixture;
		TWeakObjectPtr<AArenaGroundAreaEffect> Field;
		TWeakObjectPtr<UNiagaraComponent> AirborneVisual;
		TWeakObjectPtr<UTextureRenderTarget2D> RenderTarget;
		TWeakObjectPtr<AActor> CameraActor;
		double StartedAt = 0.;
		double LandedAt = -1.;
		bool bSpawned = false;
		bool bBurstCaptured = false;
		bool bFallingCaptured = false;
		bool bLingeringCaptured = false;
	};

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireVisualCaptureTest, "Arena.Incendiary.Visual.GroundFireCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFireVisualCaptureTest::RunTest(const FString&)
{
	if (!FApp::CanEverRender())
	{
		AddInfo(TEXT("Run with -RenderOffscreen instead of -NullRHI to capture ground-fire visuals."));
		return true;
	}

	if (GEditor->PlayWorld)
	{
		AddError(TEXT("Visual regression requires an isolated editor."));
		return false;
	}

	TSharedRef<FGroundFireCaptureFixture> Fixture = MakeShared<FGroundFireCaptureFixture>();
	GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = FSoftClassPath(UGameInstance::StaticClass());
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>();
	Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
	Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Floor->GetStaticMeshComponent()->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial")));
	Floor->SetActorScale3D(FVector(30, 30, 1));
	Floor->SetActorLocation(FVector(0, 0, -50));
	ADirectionalLight* Light = World->SpawnActor<ADirectionalLight>();
	Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Light->SetActorRotation(FRotator(-50, -20, 0));
	Light->GetLightComponent()->SetIntensity(3.f);
	World->SpawnActor<APlayerStart>(FVector(900, 900, 200), FRotator::ZeroRotator);
	ULevelEditorPlaySettings* PlaySettings = NewObject<ULevelEditorPlaySettings>();
	PlaySettings->SetPlayNetMode(PIE_Standalone);
	PlaySettings->SetPlayNumberOfClients(1);
	PlaySettings->SetRunUnderOneProcess(true);
	PlaySettings->SetClientWindowSize(FIntPoint(1024, 768));
	PlaySettings->AddToRoot();
	FRequestPlaySessionParams Params;
	Params.EditorPlaySettings = PlaySettings;
	Params.GameModeOverride = AGameModeBase::StaticClass();
	Params.bAllowOnlineSubsystem = false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
	ADD_LATENT_AUTOMATION_COMMAND(FCaptureGroundFire(this, Fixture));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
