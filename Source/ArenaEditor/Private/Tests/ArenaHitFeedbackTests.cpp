// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaHitFeedbackTestController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "ArenaGameplayTags.h"
#include "Actor/ArenaGroundAreaDefinition.h"
#include "Actor/ArenaGroundAreaEffect.h"
#include "Components/BoxComponent.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Tests/ArenaIncendiaryTestPawn.h"
#include "UI/ArenaReticleWidget.h"

void AArenaHitFeedbackTestController::ClientNotifyEnemyHit_Implementation(EArenaHitFeedbackType FeedbackType)
{
	ReceivedFeedback.Add(FeedbackType);
}

void AArenaHitFeedbackTestController::SetupInputComponent()
{
	APlayerController::SetupInputComponent();
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaHitFeedbackOutcomesTest, "Arena.UI.HitFeedback.DamageOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaHitFeedbackOutcomesTest::RunTest(const FString& Parameters)
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
	AArenaIncendiaryTestPawn* Source = World->SpawnActor<AArenaIncendiaryTestPawn>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AArenaIncendiaryTestPawn* Target = World->SpawnActor<AArenaIncendiaryTestPawn>(FVector(1000, 0, 0), FRotator::ZeroRotator, Spawn);
	Target->Team = FGenericTeamId(2);
	AArenaHitFeedbackTestController* Controller = World->SpawnActor<AArenaHitFeedbackTestController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	Instance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Source);
	Source->AbilitySystem->InitAbilityActorInfo(Source, Source);
	Target->AbilitySystem->InitAbilityActorInfo(Target, Target);
	Source->AbilitySystem->AddAttributeSetSubobject(Source->Health.Get());
	Target->AbilitySystem->AddAttributeSetSubobject(Target->Health.Get());

	auto Damage = [Source, Target](float Magnitude, bool bDirectHit)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>();
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UArenaHealthSet::GetDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Magnitude);
		FGameplayEffectSpec Spec(Effect, Source->AbilitySystem->MakeEffectContext(), 1.0f);
		if (bDirectHit)
		{
			Spec.AddDynamicAssetTag(ArenaGameplayTags::DamageType_Grenade_DirectHit);
		}
		Source->AbilitySystem->ApplyGameplayEffectSpecToTarget(Spec, Target->AbilitySystem);
	};

	Damage(10.0f, false);
	TestEqual(TEXT("Nonlethal damage removes health"), Target->Health->GetHealth(), 90.0f);
	TestEqual(TEXT("One hit produces one feedback"), Controller->ReceivedFeedback.Num(), 1);
	if (Controller->ReceivedFeedback.Num() == 1)
	{
		TestTrue(TEXT("Nonlethal damage reports Hit"), Controller->ReceivedFeedback.Last() == EArenaHitFeedbackType::Hit);
	}
	Damage(90.0f, false);
	TestEqual(TEXT("Ordinary kill produces one additional feedback"), Controller->ReceivedFeedback.Num(), 2);
	if (Controller->ReceivedFeedback.Num() == 2)
	{
		TestTrue(TEXT("Ordinary lethal damage reports Kill"), Controller->ReceivedFeedback.Last() == EArenaHitFeedbackType::Kill);
	}
	Damage(100.0f, true);
	TestEqual(TEXT("Damage to a dead target does not replay a kill"), Controller->ReceivedFeedback.Num(), 2);

	Target->AbilitySystem->SetNumericAttributeBase(UArenaHealthSet::GetHealthAttribute(), 100.0f);
	Damage(10.0f, true);
	TestEqual(TEXT("Nonlethal direct hit is still a hit"), Controller->ReceivedFeedback.Num(), 3);
	if (Controller->ReceivedFeedback.Num() == 3)
	{
		TestTrue(TEXT("Direct damage without death does not report InstantKill"), Controller->ReceivedFeedback.Last() == EArenaHitFeedbackType::Hit);
	}
	Damage(100.0f, true);
	TestEqual(TEXT("Direct kill produces one additional feedback"), Controller->ReceivedFeedback.Num(), 4);
	if (Controller->ReceivedFeedback.Num() == 4)
	{
		TestTrue(TEXT("Lethal direct damage reports InstantKill"), Controller->ReceivedFeedback.Last() == EArenaHitFeedbackType::InstantKill);
	}

	Target->AbilitySystem->SetNumericAttributeBase(UArenaHealthSet::GetHealthAttribute(), 100.0f);
	Target->AbilitySystem->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Gameplay.DamageImmunity")));
	Damage(100.0f, true);
	TestEqual(TEXT("Immunity does not produce feedback"), Controller->ReceivedFeedback.Num(), 4);
	Target->AbilitySystem->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Gameplay.DamageImmunity")));
	Target->Team = Source->Team;
	Damage(10.0f, false);
	TestEqual(TEXT("Ally damage does not produce enemy feedback"), Controller->ReceivedFeedback.Num(), 4);

	Target->Team = FGenericTeamId(2);
	Target->AbilitySystem->SetNumericAttributeBase(UArenaHealthSet::GetHealthAttribute(), 100.0f);
	Target->AbilitySystem->AddLooseGameplayTag(ArenaGameplayTags::DamageType_Grenade_DirectHit);
	Damage(100.0f, false);
	TestEqual(TEXT("Ordinary damage with a tagged target produces one feedback"), Controller->ReceivedFeedback.Num(), 5);
	if (Controller->ReceivedFeedback.Num() == 5)
	{
		TestTrue(TEXT("Target tags cannot turn ordinary damage into a direct kill"), Controller->ReceivedFeedback.Last() == EArenaHitFeedbackType::Kill);
	}
	Target->AbilitySystem->RemoveLooseGameplayTag(ArenaGameplayTags::DamageType_Grenade_DirectHit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaHitFeedbackIncendiaryTest, "Arena.UI.HitFeedback.ProductionIncendiary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaHitFeedbackIncendiaryTest::RunTest(const FString& Parameters)
{
	const UArenaGroundAreaDefinition* Definition = LoadObject<UArenaGroundAreaDefinition>(nullptr,
		TEXT("/Game/Blueprints/Weapon/Data/GrenadeData/SecondaryGrenade/DA_IncendiaryGroundArea.DA_IncendiaryGroundArea"));
	UClass* ReticleClass = LoadClass<UArenaReticleWidget>(nullptr, TEXT("/Game/UI/W_Reticle.W_Reticle_C"));
	UMaterialInterface* KillMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/UI/Hud/HitFeedback/MI_UI_Feedback_Kill.MI_UI_Feedback_Kill"));
	if (!TestNotNull(TEXT("Production incendiary definition loads"), Definition) ||
		!TestNotNull(TEXT("Production reticle loads"), ReticleClass) || !TestNotNull(TEXT("Ordinary kill material loads"), KillMaterial))
	{
		return false;
	}

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

	AActor* Floor = World->SpawnActor<AActor>();
	UBoxComponent* FloorShape = NewObject<UBoxComponent>(Floor);
	Floor->SetRootComponent(FloorShape);
	FloorShape->InitBoxExtent(FVector(2000.0f, 2000.0f, 10.0f));
	FloorShape->SetCollisionObjectType(ECC_WorldStatic);
	FloorShape->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FloorShape->SetCollisionResponseToAllChannels(ECR_Block);
	Floor->SetActorLocation(FVector(0.0f, 0.0f, -10.0f));
	FloorShape->RegisterComponent();
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArenaIncendiaryTestPawn* Source = World->SpawnActor<AArenaIncendiaryTestPawn>(FVector(-1000.0f, 0.0f, 60.0f), FRotator::ZeroRotator, Spawn);
	AArenaIncendiaryTestPawn* Target = World->SpawnActor<AArenaIncendiaryTestPawn>(FVector(0.0f, 0.0f, 60.0f), FRotator::ZeroRotator, Spawn);
	Target->Team = FGenericTeamId(2);
	AArenaHitFeedbackTestController* Controller = World->SpawnActor<AArenaHitFeedbackTestController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	Instance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Source);
	Source->AbilitySystem->InitAbilityActorInfo(Source, Source);
	Target->AbilitySystem->InitAbilityActorInfo(Target, Target);
	Source->AbilitySystem->AddAttributeSetSubobject(Source->Health.Get());
	Target->AbilitySystem->AddAttributeSetSubobject(Target->Health.Get());
	Target->AbilitySystem->SetNumericAttributeBase(UArenaHealthSet::GetHealthAttribute(), 10.0f);
	Target->AbilitySystem->AddLooseGameplayTag(ArenaGameplayTags::DamageType_Grenade_DirectHit);

	FArenaGroundAreaSettings Settings = Definition->Settings;
	Settings.bFallToGround = false;
	const FTransform Placement(FVector(0.0f, 0.0f, 2.0f));
	AArenaGroundAreaEffect* Area = World->SpawnActorDeferred<AArenaGroundAreaEffect>(AArenaGroundAreaEffect::StaticClass(),
		Placement, nullptr, Source, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Area->Initialize(Settings);
	Area->FinishSpawning(Placement);
	if (!Area->HasActorBegunPlay())
	{
		Area->DispatchBeginPlay();
	}
	for (int32 Frame = 0; Frame < 100 && Target->Health->GetHealth() > 0.0f; ++Frame)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.02f);
	}
	TestEqual(TEXT("Production ground fire burns the target to death"), Target->Health->GetHealth(), 0.0f);
	if (TestTrue(TEXT("Production fire sends damage feedback"), Controller->ReceivedFeedback.Num() >= 2))
	{
		TestTrue(TEXT("Nonlethal production fire reports Hit"), Controller->ReceivedFeedback[0] == EArenaHitFeedbackType::Hit);
		TestTrue(TEXT("Lethal production fire reports ordinary Kill"), Controller->ReceivedFeedback.Last() == EArenaHitFeedbackType::Kill);
		TestFalse(TEXT("Ground fire never reports a direct kill"), Controller->ReceivedFeedback.Contains(EArenaHitFeedbackType::InstantKill));
		UArenaReticleWidget* Reticle = CreateWidget<UArenaReticleWidget>(Controller, ReticleClass);
		Reticle->TakeWidget();
		Reticle->ShowEnemyHit(Controller->ReceivedFeedback.Last());
		if (UFunction* LegacyElimination = Reticle->FindFunction(TEXT("Eliminate")))
		{
			Reticle->ProcessEvent(LegacyElimination, nullptr);
			Reticle->FlushAnimations();
			TestFalse(TEXT("Generic elimination must not start legacy animations"), Reticle->IsAnyAnimationPlaying());
		}
		UImage* LegacyMarker = Cast<UImage>(Reticle->GetWidgetFromName(TEXT("EliminationMarker")));
		if (TestNotNull(TEXT("Legacy elimination image exists"), LegacyMarker))
		{
			TestTrue(TEXT("Generic elimination cannot reveal an extra kill marker"), LegacyMarker->GetVisibility() == ESlateVisibility::Collapsed);
		}
		UImage* Feedback = Cast<UImage>(Reticle->GetWidgetFromName(TEXT("FeedbackFX")));
		if (TestNotNull(TEXT("Production feedback image is bound"), Feedback))
		{
			UMaterialInstanceDynamic* Material = Feedback->GetDynamicMaterial();
			if (TestNotNull(TEXT("WBP selects a feedback material"), Material))
			{
				TestEqual(TEXT("Fire kill selects the shortened ordinary-kill material"), Material->Parent.Get(), KillMaterial);
			}
		}
		Reticle->RemoveFromParent();
	}
	return true;
}
#endif
