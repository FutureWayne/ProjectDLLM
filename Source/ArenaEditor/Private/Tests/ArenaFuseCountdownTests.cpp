// Copyright Ludens Studio. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Player/ArenaPlayerState.h"
#include "Tests/ArenaUIColorTestController.h"
#include "UI/ArenaReticleWidget.h"
#include "UI/ArenaUIColorLibrary.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaFuseCountdownTest, "Arena.UI.FuseCountdown.FormattingAndCriticalBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaFuseCountdownTest::RunTest(const FString& Parameters)
{
	UGameInstance* Instance = NewObject<UGameInstance>(GEngine);
	Instance->InitializeStandalone();
	UWorld* World = Instance->GetWorld();
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	ON_SCOPE_EXIT
	{
		Instance->Shutdown();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};

	AArenaUIColorTestController* Controller = World->SpawnActor<AArenaUIColorTestController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	Instance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	AArenaPlayerState* Player = World->SpawnActor<AArenaPlayerState>();
	Controller->SetPlayerState(Player);
	if (UFunction* OnRep = Controller->FindFunction(TEXT("OnRep_PlayerState")))
	{
		Controller->ProcessEvent(OnRep, nullptr);
	}
	Controller->Possess(World->SpawnActor<APawn>());
	UClass* ReticleClass = LoadClass<UArenaReticleWidget>(nullptr, TEXT("/Game/UI/W_Reticle.W_Reticle_C"));
	if (!TestNotNull(TEXT("Production reticle loads"), ReticleClass))
	{
		return false;
	}

	struct FCountdownCase
	{
		double Remaining;
		const TCHAR* Text;
		bool bCritical;
	};
	const FCountdownCase Cases[] =
	{
		{ 1.8, TEXT("1.80"), false },
		{ 1.0, TEXT("1.00"), false },
		{ 0.999, TEXT("0.99"), true },
		{ 0.7, TEXT("0.70"), true },
		{ 0.01, TEXT("0.01"), true },
		{ 2.0, TEXT("2.00"), false },
	};
	for (int32 TeamId : { 1, 2 })
	{
		Player->SetGenericTeamId(FGenericTeamId(TeamId));
		UArenaReticleWidget* Reticle = CreateWidget<UArenaReticleWidget>(Controller, ReticleClass);
		TSharedPtr<SWidget> Slate = Reticle->TakeWidget();
		UTextBlock* Seconds = Cast<UTextBlock>(Reticle->GetWidgetFromName(TEXT("FuseSeconds")));
		UImage* Ticks = Cast<UImage>(Reticle->GetWidgetFromName(TEXT("Fill")));
		UWidget* Panel = Reticle->GetWidgetFromName(TEXT("FusePanel"));
		UFunction* ADS = Reticle->FindFunction(TEXT("ADS"));
		UFunction* Tick = Reticle->FindFunction(TEXT("TickFuseCountdown"));
		if (!TestNotNull(TEXT("Fuse numeric readout exists"), Seconds) ||
			!TestNotNull(TEXT("Fuse tick material exists"), Ticks) ||
			!TestNotNull(TEXT("Fuse panel exists"), Panel) ||
			!TestNotNull(TEXT("Production ADS entry exists"), ADS) ||
			!TestNotNull(TEXT("Fuse presentation tick exists"), Tick))
		{
			return false;
		}
		TestEqual(TEXT("Initially hidden until a grenade is held"), Panel->GetVisibility(), ESlateVisibility::Collapsed);
		const FBoolProperty* OnProperty = FindFProperty<FBoolProperty>(ADS, TEXT("ON"));
		const FNumericProperty* DurationProperty = FindFProperty<FNumericProperty>(ADS, TEXT("DetonationTimer"));
		if (!TestNotNull(TEXT("ADS active input exists"), OnProperty) || !TestNotNull(TEXT("ADS fuse duration input exists"), DurationProperty))
		{
			return false;
		}
		for (const FCountdownCase& Case : Cases)
		{
			FStructOnScope Args(ADS);
			OnProperty->SetPropertyValue_InContainer(Args.GetStructMemory(), true);
			DurationProperty->SetFloatingPointPropertyValue(DurationProperty->ContainerPtrToValuePtr<void>(Args.GetStructMemory()), Case.Remaining);
			Reticle->ProcessEvent(ADS, Args.GetStructMemory());
			TestEqual(TEXT("Countdown uses exactly two fractional digits"), Seconds->GetText().ToString(), FString(Case.Text));
			TestEqual(TEXT("Digits switch only below one second"), Seconds->GetColorAndOpacity().GetSpecifiedColor(),
				UArenaUIColorLibrary::GetSharedColor(Case.bCritical ? EArenaSharedColorRole::Critical : EArenaSharedColorRole::Text));
			UMaterialInstanceDynamic* Material = Ticks->GetDynamicMaterial();
			if (TestNotNull(TEXT("Tick material is available"), Material))
			{
				float Critical = -1.0f;
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Critical")), Critical);
				TestEqual(TEXT("Tick color uses the same critical boundary"), Critical, Case.bCritical ? 1.0f : 0.0f);
				FLinearColor Primary;
				Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Primary")), Primary);
				TestEqual(TEXT("Normal timer color remains the owner's faction color"), Primary,
					UArenaUIColorLibrary::GetTeamColor(TeamId, EArenaFactionColorRole::Primary));
			}
		}
		FStructOnScope Release(ADS);
		OnProperty->SetPropertyValue_InContainer(Release.GetStructMemory(), false);
		Reticle->ProcessEvent(ADS, Release.GetStructMemory());
		World->DeltaTimeSeconds = 0.2f;
		Reticle->ProcessEvent(Tick, nullptr);
		TestEqual(TEXT("Release completes the fade and hides the countdown"), Panel->GetVisibility(), ESlateVisibility::Collapsed);
		Reticle->ReleaseSlateResources(true);
		Slate.Reset();
	}
	return true;
}
#endif
