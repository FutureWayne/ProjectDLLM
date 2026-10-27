// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaHitFeedbackTestController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "ArenaGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Tests/ArenaIncendiaryTestPawn.h"

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
	return true;
}
#endif
