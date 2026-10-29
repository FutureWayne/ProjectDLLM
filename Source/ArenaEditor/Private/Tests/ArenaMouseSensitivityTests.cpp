// Copyright Ludens Studio. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/ArenaAbilitySet.h"
#include "AbilitySystem/ArenaAbilitySystemComponent.h"
#include "AbilitySystem/ArenaGameplayAbility.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Player/ArenaLocalPlayer.h"
#include "Player/ArenaPlayerController.h"
#include "Player/ArenaPlayerState.h"
#include "Settings/ArenaSettingsShared.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaMouseSensitivityTest, "Arena.Input.MouseSensitivity.SettingsToLookInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMouseSensitivityTest::RunTest(const FString& Parameters)
{
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	ON_SCOPE_EXIT
	{
		World->BeginTearingDown();
		Instance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};

	UClass* ControllerClass = LoadClass<AArenaPlayerController>(nullptr,
		TEXT("/Game/Blueprints/PlayerController/B_ArenaPlayerController.B_ArenaPlayerController_C"));
	if (!TestNotNull(TEXT("Production controller loads"), ControllerClass))
	{
		return false;
	}

	AArenaPlayerController* Controller = World->SpawnActor<AArenaPlayerController>(ControllerClass);
	UArenaLocalPlayer* LocalPlayer = NewObject<UArenaLocalPlayer>(GEngine);
	Instance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	APawn* Pawn = World->SpawnActor<APawn>();
	Controller->Possess(Pawn);
	UArenaSettingsShared* Settings = LocalPlayer->GetSharedSettings();

	// Use the reflected setters used by the settings screen, without writing the user's save slot.
	auto SetScalar = [Settings](FName Name, double Value)
	{
		Settings->ProcessEvent(Settings->FindFunctionChecked(Name), &Value);
	};
	auto SetToggle = [Settings](FName Name, bool bValue)
	{
		Settings->ProcessEvent(Settings->FindFunctionChecked(Name), &bValue);
	};
	auto Look = [Controller]()
	{
		Controller->RotationInput = FRotator::ZeroRotator;
		Controller->Input_LookMouse(FInputActionValue(FVector2D(2.0, 3.0)));
		return Controller->RotationInput;
	};

	SetScalar(TEXT("SetMouseSensitivityX"), 1.0);
	SetScalar(TEXT("SetMouseSensitivityY"), 1.0);
	SetToggle(TEXT("SetInvertHorizontalAxis"), false);
	SetToggle(TEXT("SetInvertVerticalAxis"), false);
	const FRotator Baseline = Look();
	TestTrue(TEXT("Mouse input reaches yaw"), !FMath::IsNearlyZero(Baseline.Yaw));
	TestTrue(TEXT("Mouse input reaches pitch"), !FMath::IsNearlyZero(Baseline.Pitch));

	SetScalar(TEXT("SetMouseSensitivityX"), 2.5);
	SetScalar(TEXT("SetMouseSensitivityY"), 0.25);
	const FRotator Scaled = Look();
	TestEqual(TEXT("X sensitivity scales yaw immediately"), Scaled.Yaw, Baseline.Yaw * 2.5);
	TestEqual(TEXT("Y sensitivity scales pitch independently"), Scaled.Pitch, Baseline.Pitch * 0.25);

	SetToggle(TEXT("SetInvertHorizontalAxis"), true);
	const FRotator HorizontalInverted = Look();
	TestEqual(TEXT("Horizontal inversion reverses yaw"), HorizontalInverted.Yaw, -Scaled.Yaw);
	TestEqual(TEXT("Horizontal inversion preserves pitch"), HorizontalInverted.Pitch, Scaled.Pitch);
	SetToggle(TEXT("SetInvertVerticalAxis"), true);
	const FRotator BothInverted = Look();
	TestEqual(TEXT("Vertical inversion reverses pitch"), BothInverted.Pitch, -Scaled.Pitch);

	TArray<uint8> SaveData;
	TestTrue(TEXT("Mouse settings serialize"), UGameplayStatics::SaveGameToMemory(Settings, SaveData));
	USaveGame* Reloaded = UGameplayStatics::LoadGameFromMemory(SaveData);
	if (TestNotNull(TEXT("Mouse settings deserialize"), Reloaded))
	{
		double ReloadedValue = 0.0;
		Reloaded->ProcessEvent(Reloaded->FindFunctionChecked(TEXT("GetMouseSensitivityX")), &ReloadedValue);
		TestEqual(TEXT("Saved X sensitivity survives reload"), ReloadedValue, 2.5);
		Reloaded->ProcessEvent(Reloaded->FindFunctionChecked(TEXT("GetMouseSensitivityY")), &ReloadedValue);
		TestEqual(TEXT("Saved Y sensitivity survives reload"), ReloadedValue, 0.25);
	}

	SetScalar(TEXT("SetMouseSensitivityX"), 1.0);
	SetScalar(TEXT("SetMouseSensitivityY"), 1.0);
	SetToggle(TEXT("SetInvertHorizontalAxis"), false);
	SetToggle(TEXT("SetInvertVerticalAxis"), false);
	TestEqual(TEXT("Reverting settings restores look input"), Look(), Baseline);

	Controller->SetIgnoreLookInput(true);
	TestEqual(TEXT("Ignored look input stays blocked"), Look(), FRotator::ZeroRotator);
	Controller->ResetIgnoreLookInput();
	Controller->UnPossess();
	TestEqual(TEXT("No pawn produces no rotation"), Look(), FRotator::ZeroRotator);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaSecondaryGrenadeTargetingTest, "Arena.Input.MouseSensitivity.SecondaryGrenadeTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaSecondaryGrenadeTargetingTest::RunTest(const FString& Parameters)
{
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	ON_SCOPE_EXIT
	{
		World->BeginTearingDown();
		Instance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};

	UClass* ControllerClass = LoadClass<AArenaPlayerController>(nullptr,
		TEXT("/Game/Blueprints/PlayerController/B_ArenaPlayerController.B_ArenaPlayerController_C"));
	if (!TestNotNull(TEXT("Production controller loads"), ControllerClass))
	{
		return false;
	}
	AArenaPlayerController* Controller = World->SpawnActor<AArenaPlayerController>(ControllerClass);
	UArenaLocalPlayer* LocalPlayer = NewObject<UArenaLocalPlayer>(GEngine);
	Instance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	APawn* Pawn = World->SpawnActor<APawn>();
	Controller->Possess(Pawn);
	AArenaPlayerState* PlayerState = World->SpawnActor<AArenaPlayerState>();
	PlayerState->SetOwner(Controller);
	Controller->SetPlayerState(PlayerState);
	UArenaAbilitySystemComponent* ASC = PlayerState->GetArenaAbilitySystemComponent();
	ASC->InitAbilityActorInfo(PlayerState, Pawn);

	const FGameplayTag SecondaryInput = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Grenade.Special"));
	const FGameplayTag PrimaryInput = FGameplayTag::RequestGameplayTag(TEXT("InputTag.Grenade.Primary"));
	const UArenaAbilitySet* SecondarySet = LoadObject<UArenaAbilitySet>(nullptr,
		TEXT("/Game/Blueprints/AbilitySystem/Data/DA_GrenadeSecondary.DA_GrenadeSecondary"));
	if (!TestNotNull(TEXT("Production secondary ability set loads"), SecondarySet))
	{
		return false;
	}
	SecondarySet->GiveToAbilitySystem(ASC, nullptr);
	bool bFoundSecondaryInput = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		bFoundSecondaryInput |= Spec.GetDynamicSpecSourceTags().HasTagExact(SecondaryInput);
	}
	TestTrue(TEXT("Production Secondary Grenade uses the targeting input tag"), bFoundSecondaryInput);
	ASC->ClearAllAbilities();

	const UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(nullptr,
		TEXT("/Game/Blueprints/Input/IMC_Default.IMC_Default"));
	bool bFoundRightMouse = false;
	if (TestNotNull(TEXT("Production input mapping loads"), MappingContext))
	{
		for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
		{
			bFoundRightMouse |= Mapping.Key == EKeys::RightMouseButton && Mapping.Action &&
				Mapping.Action->GetFName() == TEXT("IA_GrenadeSpecial");
		}
	}
	TestTrue(TEXT("Right mouse is mapped to Secondary Grenade"), bFoundRightMouse);

	// Exercise real GAS activation and input lifetimes without grenade world/animation dependencies.
	FGameplayAbilitySpec SecondarySpec(UArenaGameplayAbility::StaticClass(), 1);
	SecondarySpec.GetDynamicSpecSourceTags().AddTag(SecondaryInput);
	const FGameplayAbilitySpecHandle SecondaryHandle = ASC->GiveAbility(SecondarySpec);
	FGameplayAbilitySpec PrimarySpec(UArenaGameplayAbility::StaticClass(), 1);
	PrimarySpec.GetDynamicSpecSourceTags().AddTag(PrimaryInput);
	ASC->GiveAbility(PrimarySpec);
	UArenaSettingsShared* Settings = LocalPlayer->GetSharedSettings();
	auto SetScalar = [Settings](FName Name, double Value)
	{
		Settings->ProcessEvent(Settings->FindFunctionChecked(Name), &Value);
	};
	auto Look = [Controller]()
	{
		Controller->RotationInput = FRotator::ZeroRotator;
		Controller->Input_LookMouse(FInputActionValue(FVector2D(2.0, 3.0)));
		return Controller->RotationInput;
	};
	auto Press = [Controller, ASC](FGameplayTag InputTag)
	{
		Controller->Input_AbilityInputTagPressed(InputTag);
		ASC->ProcessAbilityInput(1.0f / 60.0f, false);
	};
	auto Release = [Controller, ASC](FGameplayTag InputTag)
	{
		Controller->Input_AbilityInputTagReleased(InputTag);
		ASC->ProcessAbilityInput(1.0f / 60.0f, false);
	};
	SetScalar(TEXT("SetMouseSensitivityX"), 2.0);
	SetScalar(TEXT("SetMouseSensitivityY"), 0.5);
	SetScalar(TEXT("SetTargetingMultiplier"), 0.5);
	const FRotator Baseline = Look();
	TestTrue(TEXT("Baseline mouse input is nonzero"), !Baseline.IsNearlyZero());
	Press(PrimaryInput);
	TestTrue(TEXT("Primary ability is active"), ASC->IsAbilityInputActive(PrimaryInput));
	TestEqual(TEXT("Primary grenade does not apply targeting multiplier"), Look(), Baseline);
	Release(PrimaryInput);

	Controller->Input_AbilityInputTagPressed(SecondaryInput);
	TestEqual(TEXT("Press alone before successful activation does not scale input"), Look(), Baseline);
	ASC->ProcessAbilityInput(1.0f / 60.0f, false);
	TestTrue(TEXT("Secondary input activates through GAS"), ASC->IsAbilityInputActive(SecondaryInput));
	TestEqual(TEXT("Targeting scales both mouse axes"), Look(), Baseline * 0.5);
	SetScalar(TEXT("SetTargetingMultiplier"), 0.25);
	TestEqual(TEXT("Targeting setting changes apply during the hold"), Look(), Baseline * 0.25);
	SetScalar(TEXT("SetTargetingMultiplier"), 1.0);
	TestEqual(TEXT("Multiplier one preserves normal sensitivity"), Look(), Baseline);
	SetScalar(TEXT("SetTargetingMultiplier"), 0.5);

	Release(SecondaryInput);
	TestTrue(TEXT("Release case retains an active ability to cover post-release animation"), ASC->FindAbilitySpecFromHandle(SecondaryHandle)->IsActive());
	TestEqual(TEXT("Release restores normal sensitivity even before ability ends"), Look(), Baseline);
	Press(SecondaryInput);
	ASC->CancelAbilityHandle(SecondaryHandle);
	TestEqual(TEXT("Cancellation restores sensitivity even while input is held"), Look(), Baseline);
	Release(SecondaryInput);

	ASC->SetUserAbilityActivationInhibited(true);
	Press(SecondaryInput);
	TestFalse(TEXT("Blocked activation is inactive"), ASC->IsAbilityInputActive(SecondaryInput));
	TestEqual(TEXT("Failed activation preserves normal sensitivity"), Look(), Baseline);
	Release(SecondaryInput);
	ASC->SetUserAbilityActivationInhibited(false);
	Press(SecondaryInput);
	TestEqual(TEXT("A subsequent successful activation scales again"), Look(), Baseline * 0.5);
	ASC->ClearAbilityInput();
	TestEqual(TEXT("Clearing input cannot leave targeting sensitivity stuck"), Look(), Baseline);
	Press(SecondaryInput);
	APawn* ReplacementPawn = World->SpawnActor<APawn>();
	Controller->Possess(ReplacementPawn);
	TestEqual(TEXT("Old avatar state does not affect a replacement pawn"), Look(), Baseline);
	ASC->CancelAllAbilities();
	ASC->ClearAllAbilities();
	return true;
}

#endif
