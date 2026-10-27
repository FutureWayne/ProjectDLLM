// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaIncendiaryTestPawn.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "AbilitySystem/ArenaCombatSet.h"
#include "Actor/ArenaGroundAreaEffect.h"
#include "Actor/ArenaGroundAreaDefinition.h"
#include "ArenaGameplayTags.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "NiagaraSystem.h"
#include "Player/ArenaPlayerState.h"
#include "System/ArenaSystemStatics.h"
#include "Weapon/ArenaGrenadeBase.h"
#include "Weapon/ArenaGrenadeDefinitionData.h"

AArenaIncendiaryTestPawn::AArenaIncendiaryTestPawn()
{
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Health = CreateDefaultSubobject<UArenaHealthSet>(TEXT("Health"));
	Combat = CreateDefaultSubobject<UArenaCombatSet>(TEXT("Combat"));
	GetCapsuleComponent()->InitCapsuleSize(20.f, 60.f);
	GetCharacterMovement()->SetMovementMode(MOVE_None);
}

UArenaGroundAreaTestSlowEffect::UArenaGroundAreaTestSlowEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UArenaCombatSet::GetSpeedBuffAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FScalableFloat(-0.4f);
	Modifiers.Add(Modifier);
}

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	struct FFireWorld
	{
		UGameInstance* Instance;
		UWorld* World;

		FFireWorld()
		{
			Instance = NewObject<UGameInstance>(GEngine);
			Instance->InitializeStandalone();
			World = Instance->GetWorld();
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FFireWorld()
		{
			World->BeginTearingDown();
			World->EndPlay(EEndPlayReason::Quit);
			Instance->Shutdown();
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}

		void Tick(float Seconds = 0.02f)
		{
			const int32 Steps = FMath::Max(1, FMath::CeilToInt(Seconds / 0.02f));
			for (int32 Step = 0; Step < Steps; ++Step)
			{
				// Unreal tick functions and timers run once per engine frame, even in a synthetic world.
				++GFrameCounter;
				World->Tick(LEVELTICK_All, Seconds / Steps);
			}
		}

		AActor* Box(FVector Location, FVector Extent, FRotator Rotation = FRotator::ZeroRotator)
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UBoxComponent* Shape = NewObject<UBoxComponent>(Actor);
			Actor->SetRootComponent(Shape);
			Shape->InitBoxExtent(Extent);
			Shape->SetCollisionObjectType(ECC_WorldStatic);
			Shape->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Shape->SetCollisionResponseToAllChannels(ECR_Block);
			Actor->SetActorLocationAndRotation(Location, Rotation);
			Shape->RegisterComponent();
			return Actor;
		}

		void Floor()
		{
			Box(FVector(0, 0, -10), FVector(2000, 2000, 10));
		}

		AArenaGroundAreaEffect* Fire(FArenaGroundAreaSettings Settings = FArenaGroundAreaSettings(),
			FVector Location = FVector(0, 0, 2), APawn* Instigator = nullptr, float Yaw = 0.f)
		{
			FTransform Transform(FRotator(0.f, Yaw, 0.f), Location);
			AArenaGroundAreaEffect* Field = World->SpawnActorDeferred<AArenaGroundAreaEffect>(AArenaGroundAreaEffect::StaticClass(), Transform,
				nullptr, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Settings.SetByCallerMagnitudes.IsEmpty())
			{
				Settings.SetByCallerMagnitudes.Add(ArenaGameplayTags::SetByCaller_IncendiaryDamage, 5.f);
			}

			Settings.EffectAssetTags.AddTag(ArenaGameplayTags::DamageType_Grenade_Effect);
			Field->Initialize(Settings);
			Field->FinishSpawning(Transform);
			return Field;
		}

		void Diffuse(AArenaGroundAreaEffect* Field)
		{
			for (int32 Frame = 0; Frame < 600 && !Field->IsDiffusionComplete() && IsValid(Field); ++Frame)
			{
				Tick();
			}
		}

		AArenaIncendiaryTestPawn* Pawn(FVector Location, uint8 Team)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AArenaIncendiaryTestPawn* Actor = World->SpawnActor<AArenaIncendiaryTestPawn>(Location, FRotator::ZeroRotator, Params);
			Actor->Team = FGenericTeamId(Team);
			Actor->AbilitySystem->InitAbilityActorInfo(Actor, Actor);
			Actor->AbilitySystem->AddAttributeSetSubobject(Actor->Health.Get());
			Actor->AbilitySystem->AddAttributeSetSubobject(Actor->Combat.Get());
			return Actor;
		}
	};

	int32 Burning(const AArenaGroundAreaEffect* Fire)
	{
		int32 Count = 0;
		for (const TPair<FIntPoint, FArenaGroundCell>& Pair : Fire->GetCells())
		{
			Count += Pair.Value.IsActive();
		}

		return Count;
	}

	const FArenaGroundAreaSettings& AreaSettings(const UArenaGrenadeDefinitionData* Definition)
	{
		const UArenaGroundAreaDefinition* Area = Definition && !Definition->EffectActorsToSpawn.IsEmpty()
			? Cast<UArenaGroundAreaDefinition>(Definition->EffectActorsToSpawn[0].EffectDefinition)
			: nullptr;
		static const FArenaGroundAreaSettings Missing;
		return Area ? Area->Settings : Missing;
	}

	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireFlatTest, "Arena.Incendiary.Surface.FlatAndFrameBudget", Flags)
bool FFireFlatTest::RunTest(const FString&)
{
	FFireWorld Scene;
	Scene.Floor();
	FArenaGroundAreaSettings Settings;
	Settings.Radius = 120.f;
	Settings.CellsPerFrame = 3;
	AArenaGroundAreaEffect* Fire = Scene.Fire(Settings);
	Scene.Tick();
	TestTrue(TEXT("At most three logical checks in first frame"), Fire->GetCells().Num() <= 3);
	TestFalse(TEXT("A budgeted field takes multiple frames"), Fire->IsDiffusionComplete());
	Scene.Diffuse(Fire);
	TestTrue(TEXT("Diffusion completes"), Fire->IsDiffusionComplete());
	TestEqual(TEXT("Strict 120cm circle at 30cm spacing has 45 cells"), Burning(Fire), 45);
	TestTrue(TEXT("Center burns above floor"), Fire->IsInArea(FVector(0, 0, 60), 20.f, 60.f));
	TestFalse(TEXT("Upper floor is outside damage height"), Fire->IsInArea(FVector(0, 0, 240), 20.f, 60.f));
	TestTrue(TEXT("Display cells are merged"), Fire->GetVisualCells().Num() < Burning(Fire));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireWallTest, "Arena.Incendiary.Surface.WallAndCapsuleEdge", Flags)
bool FFireWallTest::RunTest(const FString&)
{
	FFireWorld Scene;
	Scene.Floor();
	Scene.Box(FVector(45, 0, 150), FVector(3, 500, 150));
	FArenaGroundAreaSettings Settings;
	Settings.Radius = 180.f;
	AArenaGroundAreaEffect* Fire = Scene.Fire(Settings);
	Scene.Diffuse(Fire);
	for (const TPair<FIntPoint, FArenaGroundCell>& Pair : Fire->GetCells())
	{
		if (Pair.Value.IsActive())
		{
			TestTrue(TEXT("No fire can cross thin wall"), Pair.Value.Location.X < 45.);
		}
	}

	TestTrue(TEXT("Capsule on near side is hit"), Fire->IsInArea(FVector(28, 0, 60), 20.f, 60.f));
	TestFalse(TEXT("Capsule edge behind thin wall fails additional LOS check"), Fire->IsInArea(FVector(60, 0, 60), 20.f, 60.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireStepTest, "Arena.Incendiary.Surface.StairsAndCliff", Flags)
bool FFireStepTest::RunTest(const FString&)
{
	{
		FFireWorld Scene;
		Scene.Floor();
		Scene.Box(FVector(545, 0, 50), FVector(500, 500, 50));
		AArenaGroundAreaEffect* Fire = Scene.Fire();
		Scene.Diffuse(Fire);
		const FArenaGroundCell* Step = Fire->GetCells().Find(FIntPoint(2, 0));
		TestTrue(TEXT("100cm step is connected"), Step && Step->IsActive() && FMath::IsNearlyEqual(Step->Location.Z, 100., 1.));
		TestTrue(TEXT("Capsule standing on step is hit"), Fire->IsInArea(FVector(90, 0, 160), 20, 60));
	}

	{
		FFireWorld Scene;
		Scene.Floor();
		Scene.Box(FVector(545, 0, 100), FVector(500, 500, 100));
		AArenaGroundAreaEffect* Fire = Scene.Fire();
		Scene.Diffuse(Fire);
		for (const TPair<FIntPoint, FArenaGroundCell>& Pair : Fire->GetCells())
		{
			if (Pair.Value.IsActive())
			{
				TestTrue(TEXT("200cm step cannot be climbed"), Pair.Value.Location.X < 45.);
			}
		}
	}

	{
		FFireWorld Scene;
		Scene.Box(FVector(-500, 0, -10), FVector(545, 1000, 10));
		Scene.Box(FVector(1000, 0, -610), FVector(1000, 1000, 10));
		AArenaGroundAreaEffect* Fire = Scene.Fire();
		Scene.Diffuse(Fire);
		for (const TPair<FIntPoint, FArenaGroundCell>& Pair : Fire->GetCells())
		{
			if (Pair.Value.IsActive())
			{
				TestTrue(TEXT("600cm cliff is outside maximum drop"), Pair.Value.Location.Z > -500.);
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireCeilingTest, "Arena.Incendiary.Surface.CeilingAndUnsupportedOrigin", Flags)
bool FFireCeilingTest::RunTest(const FString&)
{
	{
		FFireWorld Scene;
		Scene.Floor();
		Scene.Box(FVector(0, 0, 45), FVector(1000, 1000, 10));
		AArenaGroundAreaEffect* Fire = Scene.Fire();
		Scene.Diffuse(Fire);
		TestEqual(TEXT("Low-clearance cells immediately stop damage"), Burning(Fire), 0);
		TestTrue(TEXT("Low-clearance visuals are excluded from sync"), Fire->GetVisualCells().IsEmpty());
		TestFalse(TEXT("No damage under low ceiling"), Fire->IsInArea(FVector(0, 0, 30), 10, 30));
	}

	{
		FFireWorld Scene;
		AArenaGroundAreaEffect* Fire = Scene.Fire();
		Scene.Diffuse(Fire);
		TestFalse(TEXT("Unsupported area is destroyed without floating visuals or gameplay"), IsValid(Fire));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireDamageTest, "Arena.Incendiary.Damage.TeamsImmunityAndCadence", Flags)
bool FFireDamageTest::RunTest(const FString&)
{
	FFireWorld Scene;
	Scene.Floor();
	AArenaIncendiaryTestPawn* Source = Scene.Pawn(FVector(0, 0, 60), 1);
	AArenaIncendiaryTestPawn* Enemy = Scene.Pawn(FVector(90, 0, 60), 2);
	AArenaIncendiaryTestPawn* Ally = Scene.Pawn(FVector(-90, 0, 60), 1);
	AArenaIncendiaryTestPawn* Immune = Scene.Pawn(FVector(0, 90, 60), 2);
	Immune->AbilitySystem->AddLooseGameplayTag(TAG_Gameplay_DamageImmunity);
	FArenaGroundAreaSettings Settings;
	Settings.GameplayEffect = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/Blueprints/AbilitySystem/GameplayEffects/GE_IncendiaryDamage.GE_IncendiaryDamage_C"));
	if (!TestNotNull(TEXT("Blueprint damage GE loads"), Settings.GameplayEffect.Get()))
	{
		return false;
	}

	AArenaGroundAreaEffect* Fire = Scene.Fire(Settings, FVector(0, 0, 2), Source);
	Scene.Diffuse(Fire);
	Scene.Tick(0.26f);
	TestEqual(TEXT("Enemy takes one 5-point period, regardless of cells overlapped"), Enemy->Health->GetHealth(), 95.f);
	TestEqual(TEXT("Ally is not damaged"), Ally->Health->GetHealth(), 100.f);
	TestEqual(TEXT("Self damage is disabled by default"), Source->Health->GetHealth(), 100.f);
	TestEqual(TEXT("Health-set immunity is respected"), Immune->Health->GetHealth(), 100.f);
	Fire->Destroy();
	Scene.Tick(0.26f);
	TestEqual(TEXT("Destroying area stops periodic damage"), Enemy->Health->GetHealth(), 95.f);
	// A designer's Blueprint modifier edit must drive gameplay, rather than a hardcoded native modifier.
	UGameplayEffect* Effect = Settings.GameplayEffect->GetDefaultObject<UGameplayEffect>();
	if (!TestTrue(TEXT("Blueprint has a damage modifier"), !Effect->Modifiers.IsEmpty()))
	{
		return false;
	}

	const FGameplayEffectModifierMagnitude SavedMagnitude = Effect->Modifiers[0].ModifierMagnitude;
	ON_SCOPE_EXIT
	{
		Effect->Modifiers[0].ModifierMagnitude = SavedMagnitude;
	};

	Effect->Modifiers[0].ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(7.f));
	AArenaGroundAreaEffect* EditedFire = Scene.Fire(Settings, FVector(0, 0, 2), Source);
	Scene.Diffuse(EditedFire);
	Scene.Tick(0.26f);
	TestEqual(TEXT("Changing the Blueprint GE modifier changes damage to seven"), Enemy->Health->GetHealth(), 88.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireExplosionDamageTest, "Arena.Incendiary.Damage.ProductionExplosionDamage", Flags)
bool FFireExplosionDamageTest::RunTest(const FString&)
{
	const UArenaGrenadeDefinitionData* Definition = LoadObject<UArenaGrenadeDefinitionData>(nullptr,
		TEXT("/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade/DA_IncendiaryGrenadeDef.DA_IncendiaryGrenadeDef"));
	if (!TestNotNull(TEXT("Production grenade definition loads"), Definition))
	{
		return false;
	}

	TestNotNull(TEXT("Production explosion keeps its damage GE"), Definition->ExplosionGameplayEffect.Get());
	TestNotNull(TEXT("Production direct hit keeps its damage GE"), Definition->DirectHitGameplayEffect.Get());
	if (!TestTrue(TEXT("Production blast has a positive radius"), Definition->DetonationRadius > 0.f))
	{
		return false;
	}

	FFireWorld Scene;
	Scene.Floor();
	const float Radius = Definition->DetonationRadius;
	const FVector Origin(0, 0, 1000);
	AArenaIncendiaryTestPawn* Source = Scene.Pawn(Origin + FVector(0, -2.f * Radius, 0), 1);
	AArenaIncendiaryTestPawn* Enemy = Scene.Pawn(Origin + FVector(0.3f * Radius, 0, 0), 2);
	AArenaIncendiaryTestPawn* Blocked = Scene.Pawn(Origin + FVector(-0.4f * Radius, 0, 0), 2);
	AArenaIncendiaryTestPawn* Immune = Scene.Pawn(Origin + FVector(0, 0.4f * Radius, 0), 2);
	AArenaIncendiaryTestPawn* Outside = Scene.Pawn(Origin + FVector(2.f * Radius, 0, 0), 2);
	AArenaIncendiaryTestPawn* GroundTarget = Scene.Pawn(FVector(0, 0, 60), 2);
	Immune->AbilitySystem->AddLooseGameplayTag(TAG_Gameplay_DamageImmunity);
	for (AArenaIncendiaryTestPawn* Target : {Enemy, Blocked, Immune, Outside, GroundTarget})
	{
		Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	}

	Scene.Box(Origin + FVector(-0.2f * Radius, 0, 0), FVector(2.f, 0.25f * Radius, 0.25f * Radius));
	FActorSpawnParameters Spawn;
	Spawn.Instigator = Source;
	Spawn.Owner = Source;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArenaGrenadeBase* Grenade = Scene.World->SpawnActor<AArenaGrenadeBase>(Definition->GrenadeClass, Origin, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Production Blueprint grenade spawns for blast test"), Grenade))
	{
		return false;
	}

	Grenade->SetGrenadeParameter(Definition);
	Grenade->FindComponentByClass<UProjectileMovementComponent>()->StopMovementImmediately();
	// No world tick: any damage here must come from detonation, before the fire can fall or pulse.
	Grenade->SetGrenadeExplosionTimer(0.f);
	TestFalse(TEXT("Expired fuse detonates immediately"), IsValid(Grenade));
	TestTrue(TEXT("Airburst immediately damages an exposed enemy"), Enemy->Health->GetHealth() < 100.f);
	TestEqual(TEXT("A wall blocks blast damage"), Blocked->Health->GetHealth(), 100.f);
	TestEqual(TEXT("Blast respects damage immunity"), Immune->Health->GetHealth(), 100.f);
	TestEqual(TEXT("Outside blast radius is safe"), Outside->Health->GetHealth(), 100.f);
	TestEqual(TEXT("Ground target is safe before fire lands"), GroundTarget->Health->GetHealth(), 100.f);

	TActorIterator<AArenaGroundAreaEffect> It(Scene.World);
	if (!TestTrue(TEXT("Damaging blast still creates ground fire"), static_cast<bool>(It)))
	{
		return false;
	}

	AArenaGroundAreaEffect* Fire = *It;
	TestTrue(TEXT("Fire starts falling from the blast point"), Fire->IsFalling());
	Scene.Diffuse(Fire);
	Scene.Tick(0.26f);
	TestTrue(TEXT("Fire lands and completes diffusion after the blast"), Fire->IsDiffusionComplete());
	TestEqual(TEXT("Ground fire retains its independent five-point pulse"), GroundTarget->Health->GetHealth(), 95.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireSelfDamageTest, "Arena.Incendiary.Damage.ProductionSelfDamage", Flags)
bool FFireSelfDamageTest::RunTest(const FString&)
{
	const UArenaGrenadeDefinitionData* Definition = LoadObject<UArenaGrenadeDefinitionData>(nullptr,
		TEXT("/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade/DA_IncendiaryGrenadeDef.DA_IncendiaryGrenadeDef"));
	if (!TestNotNull(TEXT("Production definition loads"), Definition))
	{
		return false;
	}

	TestTrue(TEXT("Production grenade explicitly enables self damage"), AreaSettings(Definition).bAffectInstigator);
	TestTrue(TEXT("Production grenade explicitly enables friendly fire"), AreaSettings(Definition).bAffectAllies);
	FFireWorld Scene;
	Scene.Floor();
	AArenaIncendiaryTestPawn* Source = Scene.Pawn(FVector(0, 0, 96), 1);
	Source->GetCapsuleComponent()->SetCapsuleSize(42.f, 96.f);
	AArenaIncendiaryTestPawn* Ally = Scene.Pawn(FVector(100, 0, 96), 1);
	AArenaIncendiaryTestPawn* ImmuneAlly = Scene.Pawn(FVector(-100, 0, 96), 1);
	ImmuneAlly->AbilitySystem->AddLooseGameplayTag(TAG_Gameplay_DamageImmunity);
	FActorSpawnParameters Spawn;
	Spawn.Instigator = Source;
	Spawn.Owner = Source;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Keep the thrower outside the blast so this test measures only ground-fire self damage.
	AArenaGrenadeBase* Grenade = Scene.World->SpawnActor<AArenaGrenadeBase>(Definition->GrenadeClass, FVector(0, 0, 1000), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Production Blueprint grenade spawns with thrower"), Grenade))
	{
		return false;
	}

	Grenade->SetGrenadeParameter(Definition);
	UProjectileMovementComponent* Movement = Grenade->FindComponentByClass<UProjectileMovementComponent>();
	Movement->StopMovementImmediately();
	Movement->ProjectileGravityScale = 0.f;
	Grenade->SetGrenadeExplosionTimer(0.01f);
	Scene.Tick(0.1f);
	TActorIterator<AArenaGroundAreaEffect> It(Scene.World);
	if (!TestTrue(TEXT("Production grenade creates ground fire"), static_cast<bool>(It)))
	{
		return false;
	}

	AArenaGroundAreaEffect* Fire = *It;
	TestEqual(TEXT("Production spawn preserves damage instigator"), Fire->GetInstigator(), static_cast<APawn*>(Source));
	for (int32 Frame = 0; Frame < 600 && !Fire->IsDiffusionComplete(); ++Frame)
	{
		Scene.Tick();
	}

	Scene.Tick(0.26f);
	TestEqual(TEXT("Thrower standing in fire takes one period through the Blueprint GE"), Source->Health->GetHealth(), 95.f);
	TestEqual(TEXT("Ally standing in production fire takes the same five-point period"), Ally->Health->GetHealth(), 95.f);
	TestEqual(TEXT("Friendly fire respects damage immunity"), ImmuneAlly->Health->GetHealth(), 100.f);
	Ally->SetActorLocation(FVector(2000, 0, 96));
	Scene.Tick(0.26f);
	TestEqual(TEXT("Leaving the fire stops friendly damage"), Ally->Health->GetHealth(), 95.f);
	Ally->SetActorLocation(FVector(100, 0, 96));
	Scene.Tick(0.26f);
	TestEqual(TEXT("Reentering the fire resumes friendly damage"), Ally->Health->GetHealth(), 90.f);
	const float SelfHealthBeforeDestroy = Source->Health->GetHealth();
	Fire->Destroy();
	Scene.Tick(0.26f);
	TestEqual(TEXT("Destroying area stops self damage immediately"), Source->Health->GetHealth(), SelfHealthBeforeDestroy);
	TestEqual(TEXT("Destroying area stops friendly damage immediately"), Ally->Health->GetHealth(), 90.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireGrenadeTest, "Arena.Incendiary.Grenade.AirburstAndGroundProjection", Flags)
bool FFireGrenadeTest::RunTest(const FString&)
{
	FFireWorld Scene;
	Scene.Floor();
	UArenaGrenadeDefinitionData* Definition = NewObject<UArenaGrenadeDefinitionData>();
	Definition->ProjectileSpeed = 0.f;
	Definition->GravityScale = 0.f;
	Definition->TimeBeforeExplosion = 0.05f;
	UArenaGroundAreaDefinition* Area = NewObject<UArenaGroundAreaDefinition>();
	Area->Settings.Radius = 120.f;
	Area->Settings.MaxVisualCount = 3;
	FEffectActorSpawnData Spawn;
	Spawn.SpawnPolicy = ESpawnPolicy::SpawnOnDetonation;
	Spawn.EffectActorClass = AArenaGroundAreaEffect::StaticClass();
	Spawn.EffectDefinition = Area;
	Spawn.SpawnCount = 3;
	Spawn.SpawnRadius = 450.f;
	Definition->EffectActorsToSpawn.Add(Spawn);
	AArenaGrenadeBase* Grenade = Scene.World->SpawnActor<AArenaGrenadeBase>(FVector(0, 0, 1000), FRotator(30, 35, 0));
	Grenade->SetGrenadeParameter(Definition);
	Scene.Tick(0.15f);
	TestFalse(TEXT("Fuse detonates and destroys grenade in midair"), IsValid(Grenade));
	int32 Count = 0;
	for (TActorIterator<AArenaGroundAreaEffect> It(Scene.World); It; ++It)
	{
		++Count;
		TestTrue(TEXT("Area projects to ground independently from airburst"), FMath::Abs(It->GetActorLocation().Z - 2.f) < 1.f);
		TestTrue(TEXT("Projection preserves yaw and clears pitch/roll"), FMath::IsNearlyEqual(It->GetActorRotation().Yaw, 35., 0.1) && FMath::IsNearlyZero(It->GetActorRotation().Pitch) && FMath::IsNearlyZero(It->GetActorRotation().Roll));
		TestEqual(TEXT("Per-area configuration reaches each instance"), It->GetSettings().Radius, 120.f);
		TestTrue(TEXT("Visual instance count is bounded independently of coverage"), It->GetVisualCells().Num() <= 3);
	}

	TestEqual(TEXT("Configurable count produces three ground patches"), Count, 3);
	// A fuse must also work with no floor within the area's projection range.
	Grenade = Scene.World->SpawnActor<AArenaGrenadeBase>(FVector(5000, 0, 1000), FRotator::ZeroRotator);
	Grenade->SetGrenadeParameter(Definition);
	Scene.Tick(0.1f);
	TestFalse(TEXT("Missing ground cannot postpone the grenade explosion"), IsValid(Grenade));
	Count = 0;
	for (TActorIterator<AArenaGroundAreaEffect> It(Scene.World); It; ++It)
	{
		++Count;
	}

	TestEqual(TEXT("No ground creates no extra floating areas"), Count, 3);
	// Impact remains a data policy, independent from area implementation.
	Definition->ImpactDetonationPolicy.AddTag(TAG_DetonationPolicy_OnImpact);
	Definition->TimeBeforeExplosion = 20.f;
	Grenade = Scene.World->SpawnActor<AArenaGrenadeBase>(FVector(0, 0, 100), FRotator::ZeroRotator);
	Grenade->SetGrenadeParameter(Definition);
	Grenade->FindComponentByClass<UProjectileMovementComponent>()->Velocity = FVector(0, 0, -500);
	Scene.Tick(0.3f);
	TestFalse(TEXT("Configured impact policy still detonates before fuse"), IsValid(Grenade));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireCookingFuseTest, "Arena.Incendiary.Grenade.CookingFuseAndHighAirburst", Flags)
bool FFireCookingFuseTest::RunTest(const FString&)
{
	const UArenaGrenadeDefinitionData* Definition = LoadObject<UArenaGrenadeDefinitionData>(nullptr,
		TEXT("/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade/DA_IncendiaryGrenadeDef.DA_IncendiaryGrenadeDef"));
	if (!TestNotNull(TEXT("Production definition loads"), Definition))
	{
		return false;
	}

	if (!TestTrue(TEXT("Production grenade has a timed fuse"), Definition->TimeBeforeExplosion > 0.f))
	{
		return false;
	}

	// Use the same spawn entry point and unmodified data as the equipped throwing ability.
	for (const float CookedTime : {0.f, 1.f})
	{
		FFireWorld Scene;
		Scene.Floor();
		AArenaIncendiaryTestPawn* Source = Scene.Pawn(FVector(500, 0, 96), 1);
		const FTransform Transform(FRotator(90, 0, 0), FVector(0, 0, 150));
		AArenaGrenadeBase* Grenade = UArenaSystemStatics::SpawnGrenadeByGrenadeDefinition(Scene.World, Transform, Definition, Source, Source);
		if (!TestNotNull(TEXT("Equipped production grenade spawns"), Grenade))
		{
			return false;
		}

		const float Remaining = Definition->TimeBeforeExplosion - CookedTime;
		if (CookedTime > 0.f)
		{
			Grenade->SetGrenadeExplosionTimer(Remaining);
		}

		Scene.Tick(Remaining - 0.1f);
		if (!TestTrue(TEXT("Airborne grenade survives until its fuse expires"), IsValid(Grenade)))
		{
			return false;
		}

		if (CookedTime > 0.f)
		{
			TestTrue(TEXT("Cooked upward throw reaches above the old 30m projection limit"), Grenade->GetActorLocation().Z > 3000.f);
		}

		Scene.Tick(0.35f);
		TestFalse(TEXT("Default and cooked upward throws both detonate on time"), IsValid(Grenade));
		int32 Fields = 0;
		for (TActorIterator<AArenaGroundAreaEffect> It(Scene.World); It; ++It)
		{
			++Fields;
			TestTrue(TEXT("High airburst creates a falling field at altitude"), It->IsFalling() && It->GetActorLocation().Z > 1000.f);
			TestEqual(TEXT("Thrower survives the area spawn chain"), It->GetInstigator(), static_cast<APawn*>(Source));
			TestTrue(TEXT("Airborne field has no ground coverage"), It->GetCells().IsEmpty() && It->GetVisualCells().IsEmpty());
		}

		TestEqual(TEXT("Each upward throw creates exactly one ground field"), Fields, 1);
		Scene.Tick(4.f);
		for (TActorIterator<AArenaGroundAreaEffect> It(Scene.World); It; ++It)
		{
			TestFalse(TEXT("Falling field lands before igniting"), It->IsFalling());
			TestTrue(TEXT("Airburst field descends vertically onto the floor"), It->GetActorLocation().Equals(FVector(0, 0, 2), 1.f));
			TestTrue(TEXT("Landed field spreads on the ground"), It->IsDiffusionComplete() && !It->GetCells().IsEmpty());
		}
	}

	for (const float ExpiredTime : {0.f, -0.1f})
	{
		FFireWorld Scene;
		Scene.Floor();
		AArenaGrenadeBase* Grenade = UArenaSystemStatics::SpawnGrenadeByGrenadeDefinition(Scene.World,
			FTransform(FVector(0, 0, 150)), Definition, nullptr, nullptr);
		if (!TestNotNull(TEXT("Grenade spawns before replacing an expired fuse"), Grenade))
		{
			return false;
		}

		Grenade->SetGrenadeExplosionTimer(ExpiredTime);
		Scene.Tick(0.2f);
		TestFalse(TEXT("Zero/negative remaining fuse detonates instead of cancelling the countdown"), IsValid(Grenade));
		int32 Fields = 0;
		for (TActorIterator<AArenaGroundAreaEffect> It(Scene.World); It; ++It)
		{
			++Fields;
		}

		TestEqual(TEXT("Expired fuse still creates its configured falling field"), Fields, 1);
		Scene.Tick(Definition->TimeBeforeExplosion + 0.2f);
		Fields = 0;
		for (TActorIterator<AArenaGroundAreaEffect> It(Scene.World); It; ++It)
		{
			++Fields;
		}

		TestEqual(TEXT("The replaced launch fuse cannot produce another explosion"), Fields, 1);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireFallingTest, "Arena.Incendiary.Grenade.FallingContactAndLifetime", Flags)
bool FFireFallingTest::RunTest(const FString&)
{
	const UArenaGrenadeDefinitionData* Definition = LoadObject<UArenaGrenadeDefinitionData>(nullptr,
		TEXT("/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade/DA_IncendiaryGrenadeDef.DA_IncendiaryGrenadeDef"));
	if (!TestNotNull(TEXT("Production grenade loads"), Definition))
	{
		return false;
	}

	FArenaGroundAreaSettings Settings = AreaSettings(Definition);
	Settings.bFallToGround = true;
	Settings.FallingEffect = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Effects/Particles/NS_Explosion/NS_IncendiaryFallingFire.NS_IncendiaryFallingFire"));
	TestNotNull(TEXT("Production falling fire has a dedicated Niagara effect"), Settings.FallingEffect.Get());
	Settings.Lifetime = 0.7f;
	Settings.DestroyDelay = 0.2f;
	FFireWorld Scene;
	Scene.Floor();
	AArenaIncendiaryTestPawn* Source = Scene.Pawn(FVector(-600, 0, 60), 1);
	AArenaIncendiaryTestPawn* Enemy = Scene.Pawn(FVector(0, 0, 60), 2);
	AArenaGroundAreaEffect* Fire = Scene.Fire(Settings, FVector(0, 0, 3000), Source);
	Scene.Tick(1.f);
	if (!TestTrue(TEXT("Fall time does not consume the short ground lifetime"), IsValid(Fire)))
	{
		return false;
	}

	TestTrue(TEXT("Fire visibly descends instead of teleporting to the floor"), Fire->IsFalling() && Fire->GetActorLocation().Z > 1000.f && Fire->GetActorLocation().Z < 3000.f);
	TestTrue(TEXT("Descent has no horizontal drift"), FMath::IsNearlyZero(Fire->GetActorLocation().X) && FMath::IsNearlyZero(Fire->GetActorLocation().Y));
	TestTrue(TEXT("Falling fire cannot create ground cells or ground visuals"), Fire->GetCells().IsEmpty() && Fire->GetVisualCells().IsEmpty());
	TestFalse(TEXT("Falling fire cannot count a pawn as in the damage area"), Fire->IsInArea(Enemy->GetActorLocation(), 20.f, 60.f));
	TestEqual(TEXT("Standing underneath airborne fire causes no damage"), Enemy->Health->GetHealth(), 100.f);
	for (int32 Frame = 0; Frame < 300 && Fire->IsFalling(); ++Frame)
	{
		Scene.Tick();
	}

	TestTrue(TEXT("First contact starts a grounded field"), IsValid(Fire) && !Fire->IsFalling() && FMath::Abs(Fire->GetActorLocation().Z - 2.f) < 1.f);
	Scene.Diffuse(Fire);
	Scene.Tick(0.26f);
	TestTrue(TEXT("Blueprint damage begins after landing"), Enemy->Health->GetHealth() < 100.f);
	Scene.Tick(0.8f);
	TestFalse(TEXT("Ground lifetime then destroys the area"), IsValid(Fire));
	const float HealthAfterExpiry = Enemy->Health->GetHealth();
	Scene.Tick(0.3f);
	TestEqual(TEXT("Expired landed fire stops damage"), Enemy->Health->GetHealth(), HealthAfterExpiry);
	// A vertical sweep must stop at the first platform, even when one tick traverses the entire gap.
	Scene.Box(FVector(0, 0, 500), FVector(100, 100, 5));
	Fire = Scene.Fire(Settings, FVector(0, 0, 1000), Source);
	Scene.World->GetWorldSettings()->MaxUndilatedFrameTime = 1.f;
	++GFrameCounter;
	Scene.World->Tick(LEVELTICK_All, 1.f);
	TestTrue(TEXT("Large frame step lands on the first platform instead of falling through it"), IsValid(Fire) && !Fire->IsFalling() && FMath::Abs(Fire->GetActorLocation().Z - 507.f) < 1.f);
	Fire->Destroy();
	Settings.GroundTraceDepth = 200.f;
	Fire = Scene.Fire(Settings, FVector(1000, 0, 1000), Source);
	Scene.Tick(1.f);
	TestFalse(TEXT("Falling beyond configured range without a surface cleans up the actor"), IsValid(Fire));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAreaEffectLifecycleTest, "Arena.Incendiary.GroundArea.DurationAndInfiniteEffectLifecycle", Flags)
bool FAreaEffectLifecycleTest::RunTest(const FString&)
{
	UArenaGroundAreaTestSlowEffect* Effect = GetMutableDefault<UArenaGroundAreaTestSlowEffect>();
	const EGameplayEffectDurationType SavedDuration = Effect->DurationPolicy;
	const FGameplayEffectModifierMagnitude SavedMagnitude = Effect->DurationMagnitude;
	ON_SCOPE_EXIT
	{
		Effect->DurationPolicy = SavedDuration;
		Effect->DurationMagnitude = SavedMagnitude;
	};

	for (const EGameplayEffectDurationType Policy : {EGameplayEffectDurationType::Infinite, EGameplayEffectDurationType::HasDuration})
	{
		Effect->DurationPolicy = Policy;
		Effect->DurationMagnitude = FScalableFloat(60.f);
		FFireWorld Scene;
		Scene.Floor();
		AArenaIncendiaryTestPawn* Source = Scene.Pawn(FVector(-90, 0, 60), 1);
		AArenaIncendiaryTestPawn* Enemy = Scene.Pawn(FVector(90, 0, 60), 2);
		const float OriginalSpeed = Enemy->Combat->GetSpeedBuff();
		UArenaGroundAreaDefinition* Definition = NewObject<UArenaGroundAreaDefinition>();
		Definition->Settings.GameplayEffect = UArenaGroundAreaTestSlowEffect::StaticClass();
		Definition->Settings.Radius = 180.f;
		Definition->Settings.Lifetime = 10.f;
		const FTransform Transform(FVector(0, 0, 1000));
		auto SpawnArea = [&]()
		{
			AArenaGroundAreaEffect* Area = Scene.World->SpawnActorDeferred<AArenaGroundAreaEffect>(AArenaGroundAreaEffect::StaticClass(), Transform, nullptr, Source);
			Area->InitializeEffect(Definition);
			Area->FinishSpawning(Transform);
			return Area;
		};

		AArenaGroundAreaEffect* Area = SpawnArea();
		Scene.Tick(0.35f);
		TestTrue(TEXT("Non-damaging slow GE applies with an empty SetByCaller map"), FMath::IsNearlyEqual(Enemy->Combat->GetSpeedBuff(), OriginalSpeed - 0.4f));
		Scene.Tick(0.6f);
		TestTrue(TEXT("Repeated area ticks do not stack the active slow"), FMath::IsNearlyEqual(Enemy->Combat->GetSpeedBuff(), OriginalSpeed - 0.4f));
		Enemy->SetActorLocation(FVector(1000, 0, 60));
		Scene.Tick(0.3f);
		TestEqual(TEXT("Leaving area removes its duration/infinite slow"), Enemy->Combat->GetSpeedBuff(), OriginalSpeed);
		Enemy->SetActorLocation(FVector(90, 0, 60));
		Scene.Tick(0.3f);
		TestTrue(TEXT("Reentry reapplies the effect"), FMath::IsNearlyEqual(Enemy->Combat->GetSpeedBuff(), OriginalSpeed - 0.4f));
		// Removing one of two overlapping areas must preserve the other's modifier.
		AArenaGroundAreaEffect* Other = SpawnArea();
		Scene.Tick(0.35f);
		Area->Destroy();
		TestTrue(TEXT("Destroy removes only the owning area's effect"), FMath::IsNearlyEqual(Enemy->Combat->GetSpeedBuff(), OriginalSpeed - 0.4f));
		Other->Destroy();
		TestEqual(TEXT("Destroying last area restores speed"), Enemy->Combat->GetSpeedBuff(), OriginalSpeed);
		Definition->Settings.Lifetime = 0.7f;
		Area = SpawnArea();
		Scene.Tick(0.35f);
		TestTrue(TEXT("Short-lived area applies effect before expiry"), FMath::IsNearlyEqual(Enemy->Combat->GetSpeedBuff(), OriginalSpeed - 0.4f));
		Scene.Tick(0.5f);
		TestEqual(TEXT("Area expiry cleans effects before delayed actor destruction"), Enemy->Combat->GetSpeedBuff(), OriginalSpeed);
		Definition->Settings.Lifetime = 10.f;
		Definition->Settings.bRemoveEffectOnExit = false;
		Area = SpawnArea();
		Scene.Tick(0.35f);
		Enemy->SetActorLocation(FVector(1000, 0, 60));
		Scene.Tick(0.3f);
		TestTrue(TEXT("Opted-in lingering effect survives exit"), FMath::IsNearlyEqual(Enemy->Combat->GetSpeedBuff(), OriginalSpeed - 0.4f));
		Enemy->SetActorLocation(FVector(90, 0, 60));
		Scene.Tick(0.3f);
		TestTrue(TEXT("Reentry does not duplicate a lingering effect"), FMath::IsNearlyEqual(Enemy->Combat->GetSpeedBuff(), OriginalSpeed - 0.4f));
		Area->Destroy();
		TestTrue(TEXT("Lingering effect follows its own GE lifetime after area destruction"), FMath::IsNearlyEqual(Enemy->Combat->GetSpeedBuff(), OriginalSpeed - 0.4f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireAssetTest, "Arena.Incendiary.Integration.ProductionAsset", Flags)
bool FFireAssetTest::RunTest(const FString&)
{
	const UArenaGrenadeDefinitionData* Definition = LoadObject<UArenaGrenadeDefinitionData>(nullptr,
		TEXT("/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade/DA_IncendiaryGrenadeDef.DA_IncendiaryGrenadeDef"));
	if (!TestNotNull(TEXT("Production grenade definition loads"), Definition))
	{
		return false;
	}

	if (!TestTrue(TEXT("Production definition has a ground-area configuration"), !Definition->EffectActorsToSpawn.IsEmpty() && Cast<UArenaGroundAreaDefinition>(Definition->EffectActorsToSpawn[0].EffectDefinition) != nullptr))
	{
		return false;
	}

	TestTrue(TEXT("Production self damage remains enabled"), AreaSettings(Definition).bAffectInstigator);
	TestNotNull(TEXT("A real flame Niagara system is configured"), AreaSettings(Definition).CellEffect.Get());
	TestTrue(TEXT("Production fire uses airborne descent"), AreaSettings(Definition).bFallToGround);
	TestTrue(TEXT("Production grenade keeps its own explosion cue"), Definition->ExplosionCueTag.IsValid());
	UClass* DamageClass = AreaSettings(Definition).GameplayEffect.Get();
	if (TestNotNull(TEXT("Damage GE is configured on production asset"), DamageClass))
	{
		TestTrue(TEXT("Damage GE is a Blueprint"), DamageClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint));
		TestTrue(TEXT("Damage GE directly inherits engine GameplayEffect"), DamageClass->GetSuperClass() == UGameplayEffect::StaticClass());
		TestTrue(TEXT("Damage GE is Instant; field owns the interval"), DamageClass->GetDefaultObject<UGameplayEffect>()->DurationPolicy == EGameplayEffectDurationType::Instant);
	}

	TestEqual(TEXT("Only one surface field effect is configured"), Definition->EffectActorsToSpawn.Num(), 1);
	if (Definition->EffectActorsToSpawn.Num() == 1)
	{
		TestTrue(TEXT("Field is native implementation"), Definition->EffectActorsToSpawn[0].EffectActorClass == AArenaGroundAreaEffect::StaticClass());
	}

	// Exercise the shipped Blueprint grenade class too: Blueprint overrides can bypass native spawn hooks.
	FFireWorld Scene;
	Scene.Floor();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArenaGrenadeBase* Grenade = Scene.World->SpawnActor<AArenaGrenadeBase>(Definition->GrenadeClass, FVector(0, 0, 1000), FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Production Blueprint grenade spawns"), Grenade))
	{
		return false;
	}

	Grenade->SetGrenadeParameter(Definition);
	Grenade->SetGrenadeExplosionTimer(0.05f);
	Scene.Tick(0.5f);
	TestFalse(TEXT("Production Blueprint honors airburst fuse"), IsValid(Grenade));
	for (TActorIterator<AArenaGroundAreaEffect> Falling(Scene.World); Falling; ++Falling)
	{
		TestTrue(TEXT("Production fire is still falling shortly after the airburst"), Falling->IsFalling());
	}

	Scene.Tick(2.f);
	TActorIterator<AArenaGroundAreaEffect> It(Scene.World);
	if (TestTrue(TEXT("Production grenade actually creates the new field"), static_cast<bool>(It)))
	{
		TestTrue(TEXT("Data asset flame effect reaches spawned field"), It->GetSettings().CellEffect == AreaSettings(Definition).CellEffect);
		TestTrue(TEXT("Data asset Blueprint damage GE reaches spawned field"), It->GetSettings().GameplayEffect == AreaSettings(Definition).GameplayEffect);
		TestEqual(TEXT("Data asset radius reaches spawned field"), It->GetSettings().Radius, AreaSettings(Definition).Radius);
		TestTrue(TEXT("Production field completes diffusion"), It->IsDiffusionComplete());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireDiagonalWallTest, "Arena.Incendiary.Surface.DiagonalWallAndRoundCapsule", Flags)
bool FFireDiagonalWallTest::RunTest(const FString&)
{
	FFireWorld Scene;
	Scene.Floor();
	Scene.Box(FVector(7, 7, 150), FVector(1, 500, 150), FRotator(0, 45, 0));
	FArenaGroundAreaSettings Settings;
	Settings.Radius = 180.f;
	AArenaGroundAreaEffect* Fire = Scene.Fire(Settings);
	Scene.Diffuse(Fire);
	TestTrue(TEXT("A production-sized capsule on the near side is hit"), Fire->IsInArea(FVector(-40, -40, 96), 42.f, 96.f));
	TestFalse(TEXT("A 42cm capsule fully behind a diagonal wall is not hit"), Fire->IsInArea(FVector(40, 40, 96), 42.f, 96.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireSourceDestroyedTest, "Arena.Incendiary.Damage.DestroyedInstigator", Flags)
bool FFireSourceDestroyedTest::RunTest(const FString&)
{
	FFireWorld Scene;
	Scene.Floor();
	AArenaIncendiaryTestPawn* Source = Scene.Pawn(FVector(-90, 0, 60), 1);
	AArenaIncendiaryTestPawn* Enemy = Scene.Pawn(FVector(90, 0, 60), 2);
	AArenaIncendiaryTestPawn* Ally = Scene.Pawn(FVector(0, 90, 60), 1);
	FArenaGroundAreaSettings Settings;
	Settings.GameplayEffect = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/Blueprints/AbilitySystem/GameplayEffects/GE_IncendiaryDamage.GE_IncendiaryDamage_C"));
	if (!TestNotNull(TEXT("Production damage effect loads"), Settings.GameplayEffect.Get()))
	{
		return false;
	}

	AArenaGroundAreaEffect* Fire = Scene.Fire(Settings, FVector(0, 0, 2), Source);
	Scene.Diffuse(Fire);
	Scene.Tick(0.26f);
	TestEqual(TEXT("Enemy is damaged before thrower disappears"), Enemy->Health->GetHealth(), 95.f);
	Source->Destroy();
	CollectGarbage(RF_NoFlags);
	TestNull(TEXT("The destroyed thrower is actually collected"), Fire->GetInstigator());
	Scene.Tick(0.26f);
	TestTrue(TEXT("Fire survives the thrower"), IsValid(Fire));
	TestEqual(TEXT("Damage continues without the thrower's Pawn or ASC"), Enemy->Health->GetHealth(), 90.f);
	TestEqual(TEXT("The captured team still protects allies"), Ally->Health->GetHealth(), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirePlayerStateContextTest, "Arena.Incendiary.Damage.PlayerStateAttribution", Flags)
bool FFirePlayerStateContextTest::RunTest(const FString&)
{
	FFireWorld Scene;
	Scene.Floor();
	AArenaIncendiaryTestPawn* Source = Scene.Pawn(FVector(-90, 0, 60), 1);
	AArenaIncendiaryTestPawn* Enemy = Scene.Pawn(FVector(90, 0, 60), 2);
	AArenaPlayerState* State = Scene.World->SpawnActor<AArenaPlayerState>();
	State->SetGenericTeamId(FGenericTeamId(1));
	Source->AbilitySystem = State->GetAbilitySystemComponent();
	Source->AbilitySystem->InitAbilityActorInfo(State, Source);
	FGameplayEffectContextHandle ObservedContext;
	const FDelegateHandle Observer = Enemy->Health->OnHealthChanged.AddLambda(
		[&ObservedContext](AActor*, AActor*, const FGameplayEffectSpec* Spec, float, float, float)
		{
			ObservedContext = Spec->GetEffectContext();
		});
	ON_SCOPE_EXIT
	{
		Enemy->Health->OnHealthChanged.Remove(Observer);
	};

	FArenaGroundAreaSettings Settings;
	Settings.GameplayEffect = LoadClass<UGameplayEffect>(nullptr,
		TEXT("/Game/Blueprints/AbilitySystem/GameplayEffects/GE_IncendiaryDamage.GE_IncendiaryDamage_C"));
	if (!TestNotNull(TEXT("Production damage effect loads"), Settings.GameplayEffect.Get()))
	{
		return false;
	}

	AArenaGroundAreaEffect* Fire = Scene.Fire(Settings, FVector(0, 0, 2), Source);
	Scene.Diffuse(Fire);
	Scene.Tick(0.26f);
	TestEqual(TEXT("Damage preserves the ASC owner's PlayerState for elimination consumers"), ObservedContext.GetOriginalInstigator(), static_cast<AActor*>(State));
	TestEqual(TEXT("The area remains the effect causer"), ObservedContext.GetEffectCauser(), static_cast<AActor*>(Fire));
	Source->Destroy();
	CollectGarbage(RF_NoFlags);
	Scene.Tick(0.26f);
	TestEqual(TEXT("PlayerState-owned source continues damage after Pawn collection"), Enemy->Health->GetHealth(), 90.f);
	TestEqual(TEXT("Posthumous damage retains the original PlayerState"), ObservedContext.GetOriginalInstigator(), static_cast<AActor*>(State));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFireAlternateParentTest, "Arena.Incendiary.Surface.AlternateConnectedParent", Flags)
bool FFireAlternateParentTest::RunTest(const FString&)
{
	for (const int32 Budget : {1, 128})
	{
		FFireWorld Scene;
		Scene.Floor();
		Scene.Box(FVector(15, 0, 150), FVector(1, 10, 150));
		FArenaGroundAreaSettings Settings;
		Settings.Radius = 180.f;
		Settings.CellsPerFrame = Budget;
		AArenaGroundAreaEffect* Fire = Scene.Fire(Settings);
		Scene.Diffuse(Fire);
		TestTrue(TEXT("Retrying connections finishes within the area lifetime"), Fire->IsDiffusionComplete());
		const FArenaGroundCell* AroundWall = Fire->GetCells().Find(FIntPoint(1, 1));
		TestTrue(TEXT("Fire reaches around the short wall"), AroundWall && AroundWall->IsActive());
		const FArenaGroundCell* FarSide = Fire->GetCells().Find(FIntPoint(1, 0));
		TestTrue(TEXT("Another connected parent activates the cell behind the short wall"), FarSide && FarSide->IsActive());
	}

	return true;
}
#endif
