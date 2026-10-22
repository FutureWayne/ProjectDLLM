// Copyright Ludens Studio. All Rights Reserved.

#include "Tests/ArenaHealthBarTestWidget.h"

#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaHealthSet.h"
#include "Character/ArenaHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

void UArenaHealthBarTestWidget::InitializeHealthbarVisuals_Implementation(float Health, float NormalizedHealth)
{
	++SnapshotCount;
	LastHealth = Health;
	LastNormalizedHealth = NormalizedHealth;
}

void UArenaHealthBarTestWidget::PresentHealthbarChange_Implementation(float OldHealth, float NewHealth, float OldNormalizedHealth, float NewNormalizedHealth, bool bHealing, bool bOutOfHealth)
{
	++ChangeCount;
	LastHealth = NewHealth;
	LastNormalizedHealth = NewNormalizedHealth;
	LastOldNormalizedHealth = OldNormalizedHealth;
	bLastHealing = bHealing;
	bLastOutOfHealth = bOutOfHealth;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaHealthBarLifecycleTest, "Arena.UI.HealthBar.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaHealthBarLifecycleTest::RunTest(const FString& Parameters)
{
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	Instance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);

	UArenaHealthBarTestWidget* Widget = CreateWidget<UArenaHealthBarTestWidget>(Controller);
	if (!TestNotNull(TEXT("Create healthbar"), Widget))
	{
		Instance->Shutdown();
		World->DestroyWorld(false);
		return false;
	}
	UClass* HealthBarClass = LoadClass<UArenaHealthBarWidget>(nullptr, TEXT("/Game/UI/Hud/HealthBar/W_Healthbar.W_Healthbar_C"));
	UArenaHealthBarWidget* PresentedBar = HealthBarClass ? CreateWidget<UArenaHealthBarWidget>(Controller, HealthBarClass) : nullptr;
	TSharedPtr<SWidget> PresentedSlateWidget;
	UMaterialInstanceDynamic* FillMaterial = nullptr;
	if (TestNotNull(TEXT("Load migrated W_Healthbar"), PresentedBar))
	{
		PresentedSlateWidget = PresentedBar->TakeWidget();
		const FObjectPropertyBase* FillProperty = FindFProperty<FObjectPropertyBase>(HealthBarClass, TEXT("BarFillMID"));
		FillMaterial = FillProperty ? Cast<UMaterialInstanceDynamic>(FillProperty->GetObjectPropertyValue_InContainer(PresentedBar)) : nullptr;
		TestNotNull(TEXT("Blueprint creates the fill material"), FillMaterial);
	}
	Widget->ConstructForTest();
	TestEqual(TEXT("No pawn clears the bar"), Widget->LastHealth, 0.0f);
	TestEqual(TEXT("No health source has finite zero progress"), Widget->LastNormalizedHealth, 0.0f);

	auto CreateHealthPawn = [World](UArenaAbilitySystemComponent*& OutASC, UArenaHealthComponent*& OutHealth)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		OutASC = NewObject<UArenaAbilitySystemComponent>(Pawn);
		Pawn->AddInstanceComponent(OutASC);
		OutASC->RegisterComponent();
		OutASC->InitAbilityActorInfo(Pawn, Pawn);
		OutASC->AddAttributeSetSubobject(NewObject<UArenaHealthSet>(Pawn));
		OutHealth = NewObject<UArenaHealthComponent>(Pawn);
		Pawn->AddInstanceComponent(OutHealth);
		OutHealth->RegisterComponent();
		OutHealth->InitializeWithAbilitySystem(OutASC);
		return Pawn;
	};

	auto ApplyAttribute = [](UArenaAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float Value)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>();
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Override;
		Modifier.ModifierMagnitude = FScalableFloat(Value);
		ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, ASC->MakeEffectContext());
	};

	UArenaAbilitySystemComponent* FirstASC = nullptr;
	UArenaHealthComponent* FirstHealth = nullptr;
	APawn* FirstPawn = CreateHealthPawn(FirstASC, FirstHealth);
	Controller->Possess(FirstPawn);
	TestEqual(TEXT("Possession initializes current health"), Widget->LastHealth, 100.0f);
	TestEqual(TEXT("Initial snapshot plays no change animation"), Widget->ChangeCount, 0);

	ApplyAttribute(FirstASC, UArenaHealthSet::GetMaxHealthAttribute(), 200.0f);
	TestEqual(TEXT("Max health changes refresh progress"), Widget->LastNormalizedHealth, 0.5f);
	TestEqual(TEXT("Max health alone plays no damage animation"), Widget->ChangeCount, 0);
	ApplyAttribute(FirstASC, UArenaHealthSet::GetHealthAttribute(), 50.0f);
	TestEqual(TEXT("Damage uses actual max health"), Widget->LastNormalizedHealth, 0.25f);
	if (FillMaterial)
	{
		TestEqual(TEXT("WBP receives normalized previous health"), FillMaterial->K2_GetScalarParameterValue(TEXT("Health_Current")), 0.5f);
		TestEqual(TEXT("WBP receives normalized updated health"), FillMaterial->K2_GetScalarParameterValue(TEXT("Health_Updated")), 0.25f);
	}
	TestEqual(TEXT("Damage starts from previous normalized health"), Widget->LastOldNormalizedHealth, 0.5f);
	TestFalse(TEXT("Damage classified in C++"), Widget->bLastHealing);
	ApplyAttribute(FirstASC, UArenaHealthSet::GetHealthAttribute(), 80.0f);
	TestTrue(TEXT("Healing classified in C++"), Widget->bLastHealing);
	if (FillMaterial)
	{
		TestEqual(TEXT("WBP selects healing material mode"), FillMaterial->K2_GetScalarParameterValue(TEXT("DamageOrHealing")), 1.0f);
	}
	ApplyAttribute(FirstASC, UArenaHealthSet::GetHealthAttribute(), 0.0f);
	TestTrue(TEXT("Lethal damage requests elimination presentation"), Widget->bLastOutOfHealth);

	UArenaAbilitySystemComponent* SecondASC = nullptr;
	UArenaHealthComponent* SecondHealth = nullptr;
	APawn* SecondPawn = CreateHealthPawn(SecondASC, SecondHealth);
	Controller->Possess(SecondPawn);
	TestEqual(TEXT("Respawn resets snapshot"), Widget->LastHealth, 100.0f);
	const int32 CountAfterRespawn = Widget->ChangeCount;
	ApplyAttribute(FirstASC, UArenaHealthSet::GetHealthAttribute(), 10.0f);
	TestEqual(TEXT("Previous pawn is unbound"), Widget->ChangeCount, CountAfterRespawn);

	Widget->DestructForTest();
	ApplyAttribute(SecondASC, UArenaHealthSet::GetHealthAttribute(), 60.0f);
	TestEqual(TEXT("Destruct unbinds health callbacks"), Widget->ChangeCount, CountAfterRespawn);
	Widget->ConstructForTest();
	TestEqual(TEXT("Reconstruct reads latest snapshot"), Widget->LastHealth, 60.0f);
	ApplyAttribute(SecondASC, UArenaHealthSet::GetHealthAttribute(), 40.0f);
	TestEqual(TEXT("Reconstruct binds only once"), Widget->ChangeCount, CountAfterRespawn + 1);
	Controller->UnPossess();
	TestEqual(TEXT("Unpossess clears stale health"), Widget->LastHealth, 0.0f);
	Widget->DestructForTest();

	PresentedSlateWidget.Reset();
	Instance->RemoveLocalPlayer(LocalPlayer);
	World->BeginTearingDown();
	World->EndPlay(EEndPlayReason::Quit);
	Instance->Shutdown();
	World->DestroyWorld(false);
	return true;
}
#endif
