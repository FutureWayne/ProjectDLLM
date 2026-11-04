// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaKillCameraTestPawn.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Camera/ArenaPlayerCameraManager.h"
#include "Character/ArenaHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Player/ArenaPlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Tests/ArenaHitFeedbackTestController.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

AArenaKillCameraTestPawn::AArenaKillCameraTestPawn()
{
	AbilitySystem = CreateDefaultSubobject<UArenaAbilitySystemComponent>(TEXT("AbilitySystem"));
	Health = CreateDefaultSubobject<UArenaHealthSet>(TEXT("Health"));
	HealthComponent = CreateDefaultSubobject<UArenaHealthComponent>(TEXT("HealthComponent"));
}

UAbilitySystemComponent* AArenaKillCameraTestPawn::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AArenaKillCameraTestPawn::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	AbilitySystem->AddAttributeSetSubobject(Health.Get());
	HealthComponent->InitializeWithAbilitySystem(AbilitySystem);
}

void AArenaKillCameraTestPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	OutResult.Location = GetActorLocation() + TestCameraOffset;
	OutResult.Rotation = GetViewRotation();
	OutResult.FOV = TestCameraFOV;
}

AArenaKillCameraTestGameMode::AArenaKillCameraTestGameMode()
{
	DefaultPawnClass = AArenaKillCameraTestPawn::StaticClass();
	PlayerStateClass = AArenaPlayerState::StaticClass();
	PlayerControllerClass = AArenaKillCameraTestController::StaticClass();
}

AArenaKillCameraTestController::AArenaKillCameraTestController()
{
	static ConstructorHelpers::FClassFinder<AArenaPlayerCameraManager> CameraClass(TEXT("/Game/Camera/B_ArenaPlayerCameraManager"));
	if (CameraClass.Succeeded())
	{
		PlayerCameraManagerClass = CameraClass.Class;
	}
}

void AArenaKillCameraTestController::BeginPlay()
{
	APlayerController::BeginPlay();
}

void AArenaKillCameraTestController::SetupInputComponent()
{
	APlayerController::SetupInputComponent();
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaKillCameraLifecycleTest, "Arena.Camera.KillCamera.DamageAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaKillCameraLifecycleTest::RunTest(const FString& Parameters)
{
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	ON_SCOPE_EXIT
	{
		World->BeginTearingDown();
		World->EndPlay(EEndPlayReason::Quit);
		Instance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArenaKillCameraTestPawn* Victim = World->SpawnActor<AArenaKillCameraTestPawn>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AArenaKillCameraTestPawn* Killer = World->SpawnActor<AArenaKillCameraTestPawn>(FVector(1500.0, 0.0, 0.0), FRotator::ZeroRotator, Spawn);
	for (AArenaKillCameraTestPawn* Pawn : {Victim, Killer})
	{
		Pawn->AbilitySystem->InitAbilityActorInfo(Pawn, Pawn);
		Pawn->AbilitySystem->AddAttributeSetSubobject(Pawn->Health.Get());
		Pawn->HealthComponent->InitializeWithAbilitySystem(Pawn->AbilitySystem);
	}
	AArenaHitFeedbackTestController* Controller = World->SpawnActor<AArenaHitFeedbackTestController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	Instance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Victim);
	AArenaPlayerCameraManager* Camera = Cast<AArenaPlayerCameraManager>(Controller->PlayerCameraManager);
	if (!TestNotNull(TEXT("Production camera manager"), Camera))
	{
		return false;
	}
	FStructProperty* OffsetProperty = FindFProperty<FStructProperty>(Camera->GetClass(), TEXT("KillCameraAnchorOffset"));
	if (!TestNotNull(TEXT("Camera anchor offset is configurable in the camera Blueprint"), OffsetProperty))
	{
		return false;
	}
	const FVector AnchorOffset(0.0, 0.0, 120.0);
	*OffsetProperty->ContainerPtrToValuePtr<FVector>(Camera) = AnchorOffset;
	Camera->UpdateCamera(0.016f);
	const FVector NormalCameraLocation = Camera->GetCameraLocation();
	const FVector ObservationLocation = NormalCameraLocation + AnchorOffset;
	UGameplayEffect* Effect = NewObject<UGameplayEffect>();
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UArenaHealthSet::GetDamageAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FScalableFloat(200.0f);
	FGameplayEffectSpec Spec(Effect, Killer->AbilitySystem->MakeEffectContext(), 1.0f);
	Killer->AbilitySystem->ApplyGameplayEffectSpecToTarget(Spec, Victim->AbilitySystem);
	TestEqual(TEXT("Real lethal damage removes health"), Victim->Health->GetHealth(), 0.0f);
	TestTrue(TEXT("Lethal GAS damage starts the owner camera through the production health path"), Camera->IsKillCameraActive());
	TestEqual(TEXT("Authority view target retains the original killer for relevancy"), Controller->GetViewTarget(), static_cast<AActor*>(Killer));
	const FRotator OriginalControlRotation = Controller->GetControlRotation();
	const FRotator BlendStart = Camera->GetCameraRotation();
	Victim->TestCameraFOV = 95.0f;
	Killer->TestCameraFOV = 45.0f;
	Camera->UpdateCamera(0.016f);
	TestTrue(TEXT("Initial transition changes rotation smoothly"),
		FMath::Abs((Camera->GetCameraRotation() - BlendStart).GetNormalized().Pitch) < 5.0f);
	TestTrue(TEXT("Camera applies the configured height offset to the original observation point"),
		Camera->GetCameraLocation().Equals(ObservationLocation, 0.01));
	TestEqual(TEXT("Victim FOV is preserved rather than blending to the killer FOV"), Camera->GetFOVAngle(), 95.0f);
	for (int32 Frame = 0; Frame < 90; ++Frame)
	{
		Camera->UpdateCamera(0.016f);
	}
	const FVector BeforeMovement = Camera->GetCameraLocation();
	const FRotator BeforeRotation = Camera->GetCameraRotation();
	Killer->SetActorLocation(FVector(4000.0, 2500.0, 250.0));
	for (int32 Frame = 0; Frame < 90; ++Frame)
	{
		Camera->UpdateCamera(0.016f);
	}
	TestTrue(TEXT("Killer movement never translates the victim camera"), Camera->GetCameraLocation().Equals(BeforeMovement));
	TestFalse(TEXT("Killer movement updates the camera direction"), Camera->GetCameraRotation().Equals(BeforeRotation));
	const FVector ExpectedDirection = (Killer->GetActorLocation() - Camera->GetCameraLocation()).GetSafeNormal();
	TestTrue(TEXT("Camera looks at the killer from the victim camera"),
		FVector::DotProduct(Camera->GetCameraRotation().Vector(), ExpectedDirection) > 0.999);
	Victim->TestCameraOffset = FVector(-420.0, 25.0, 105.0);
	Victim->TestCameraFOV = 117.0f;
	Camera->UpdateCamera(0.016f);
	TestTrue(TEXT("Death-camera offset changes never move the original observation point"),
		Camera->GetCameraLocation().Equals(ObservationLocation, 0.01));
	TestEqual(TEXT("Original death-camera FOV changes remain intact"), Camera->GetFOVAngle(), 117.0f);
	TestTrue(TEXT("Look-at never rotates the controller and moves the original camera orbit"),
		Controller->GetControlRotation().Equals(OriginalControlRotation));
	const FVector LastVictimCameraLocation = Camera->GetCameraLocation();
	Controller->UnPossess();
	Victim->Destroy();
	Camera->UpdateCamera(0.016f);
	TestTrue(TEXT("Corpse destruction preserves the death view"), Camera->IsKillCameraActive());
	TestTrue(TEXT("Corpse destruction preserves its last camera position"), Camera->GetCameraLocation().Equals(LastVictimCameraLocation));
	TestEqual(TEXT("Corpse destruction preserves its last FOV"), Camera->GetFOVAngle(), 117.0f);
	Killer->Destroy();
	Camera->UpdateCamera(0.016f);
	TestTrue(TEXT("Losing the original killer safely holds the last direction"), Camera->IsKillCameraActive());
	TestFalse(TEXT("Target destruction never produces a nonfinite view"), Camera->GetCameraLocation().ContainsNaN());
	AArenaKillCameraTestPawn* Respawn = World->SpawnActor<AArenaKillCameraTestPawn>(FVector(500.0, 0.0, 0.0), FRotator::ZeroRotator, Spawn);
	Controller->Possess(Respawn);
	Camera->UpdateCamera(0.016f);
	TestFalse(TEXT("A new living pawn exits the kill camera"), Camera->IsKillCameraActive());
	Camera->StartKillCamera(Respawn, Respawn);
	Camera->UpdateCamera(0.016f);
	TestFalse(TEXT("Suicide retains the existing death camera"), Camera->IsKillCameraActive());
	TestFalse(TEXT("Coincident suicide targets produce a finite camera"), Camera->GetCameraLocation().ContainsNaN());
	Camera->StartKillCamera(Respawn, nullptr);
	Camera->UpdateCamera(0.016f);
	TestFalse(TEXT("Environmental death retains the existing death camera"), Camera->IsKillCameraActive());
	Camera->StopKillCamera();
	Camera->StopKillCamera();
	TestFalse(TEXT("Cleanup is idempotent"), Camera->IsKillCameraActive());
	return true;
}
#endif
