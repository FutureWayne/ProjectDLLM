// Copyright Ludens Studio. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS
#include "Blueprint/UserWidget.h"
#include "CommonRichTextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Player/ArenaPlayerState.h"
#include "Settings/ArenaUIColorSettings.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "Tests/ArenaUIColorTestController.h"
#include "UI/ArenaUIColorLibrary.h"
#include "UI/ArenaReticleWidget.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaUIColorConsumerTest, "Arena.UI.Colors.Consumers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaUIColorConsumerTest::RunTest(const FString& Parameters)
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
	Player->SetGenericTeamId(FGenericTeamId(1));
	Controller->SetPlayerState(Player);
	UArenaUIColorPalette* Palette = const_cast<UArenaUIColorPalette*>(GetMutableDefault<UArenaUIColorSettings>()->GetPalette());
	if (!TestNotNull(TEXT("Shared palette loads"), Palette))
	{
		return false;
	}

	auto Invoke = [this](UObject* Object, const TCHAR* Name)
	{
		UFunction* Function = Object ? Object->FindFunction(Name) : nullptr;
		if (!TestNotNull(Name, Function))
		{
			return;
		}
		Object->ProcessEvent(Function, nullptr);
	};
	// Establish the production PlayerState subscription just as replication does on a client.
	Invoke(Controller, TEXT("OnRep_PlayerState"));
	auto CheckBorder = [this](UUserWidget* Widget, const TCHAR* Name, int32 TeamId)
	{
		UBorder* Border = Cast<UBorder>(Widget->GetWidgetFromName(Name));
		UMaterialInstanceDynamic* Material = Border ? Border->GetDynamicMaterial() : nullptr;
		if (!TestNotNull(Name, Material))
		{
			return static_cast<UMaterialInstanceDynamic*>(nullptr);
		}
		FLinearColor Value;
		TestTrue(TEXT("Border surface parameter exists"), Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Border_Inner_RGBA")), Value));
		TestEqual(TEXT("Border surface comes from the displayed team palette"), Value.CopyWithNewOpacity(1.0f),
			UArenaUIColorLibrary::GetTeamColor(TeamId, EArenaFactionColorRole::Surface));
		TestTrue(TEXT("Border edge parameter exists"), Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Border_Outer_RGBA")), Value));
		TestEqual(TEXT("Border edge comes from the displayed team palette"), Value.CopyWithNewOpacity(1.0f),
			UArenaUIColorLibrary::GetTeamColor(TeamId, EArenaFactionColorRole::Edge));
		return Material;
	};
	UClass* ScoreClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/Scoreboard/W_SB_TeamStat.W_SB_TeamStat_C"));
	if (!TestNotNull(TEXT("Production team statistics widget loads"), ScoreClass))
	{
		return false;
	}
	const FIntProperty* TeamProperty = FindFProperty<FIntProperty>(ScoreClass, TEXT("TeamId"));
	if (!TestNotNull(TEXT("Displayed scoreboard team input"), TeamProperty))
	{
		return false;
	}
	UUserWidget* Orange = CreateWidget<UUserWidget>(Controller, ScoreClass);
	UUserWidget* Cyan = CreateWidget<UUserWidget>(Controller, ScoreClass);
	TeamProperty->SetPropertyValue_InContainer(Orange, 1);
	TeamProperty->SetPropertyValue_InContainer(Cyan, 2);
	Invoke(Orange, TEXT("SetTeamStyle"));
	Invoke(Cyan, TEXT("SetTeamStyle"));
	UMaterialInstanceDynamic* OrangeHeader = CheckBorder(Orange, TEXT("HeaderBorder"), 1);
	UMaterialInstanceDynamic* CyanHeader = CheckBorder(Cyan, TEXT("HeaderBorder"), 2);
	TestNotEqual(TEXT("Simultaneous team panels have separate material instances"), OrangeHeader, CyanHeader);
	CheckBorder(Orange, TEXT("BodyBorder"), 1);
	CheckBorder(Cyan, TEXT("BodyBorder"), 2);
	CheckBorder(Orange, TEXT("HeaderBorder"), 1);
	{
		UArenaTeamSubsystem* Teams = World->GetSubsystem<UArenaTeamSubsystem>();
		const FLinearColor WorldColor = Teams->FindTeamColorFromActor(Player);
		TGuardValue<FLinearColor> SurfaceOverride(Palette->Orange.Surface, Palette->Shared.Panel);
		TGuardValue<FLinearColor> EdgeOverride(Palette->Orange.Edge, Palette->Shared.TextMuted);
		TGuardValue<FLinearColor> PrimaryOverride(Palette->Orange.Primary, Palette->Shared.Text);
		Invoke(Orange, TEXT("SetTeamStyle"));
		CheckBorder(Orange, TEXT("HeaderBorder"), 1);
		CheckBorder(Orange, TEXT("BodyBorder"), 1);
		CheckBorder(Cyan, TEXT("HeaderBorder"), 2);
		TestEqual(TEXT("UI palette edits leave the world team-color interface unchanged"), Teams->FindTeamColorFromActor(Player), WorldColor);
	}
	TeamProperty->SetPropertyValue_InContainer(Orange, 0);
	Invoke(Orange, TEXT("SetTeamStyle"));
	CheckBorder(Orange, TEXT("HeaderBorder"), 0);

	UClass* PingClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/Scoreboard/W_UserConnectionStatus.W_UserConnectionStatus_C"));
	UUserWidget* Ping = PingClass ? CreateWidget<UUserWidget>(Controller, PingClass) : nullptr;
	if (!TestNotNull(TEXT("Production connection status loads"), Ping))
	{
		return false;
	}
	UFunction* SetPlayer = Ping->FindFunction(TEXT("SetPlayerState"));
	const FObjectPropertyBase* PlayerInput = SetPlayer ? FindFProperty<FObjectPropertyBase>(SetPlayer, TEXT("PlayerState")) : nullptr;
	if (!TestNotNull(TEXT("Connection status display-object input"), PlayerInput))
	{
		return false;
	}
	FStructOnScope PlayerArgs(SetPlayer);
	PlayerInput->SetObjectPropertyValue_InContainer(PlayerArgs.GetStructMemory(), Player);
	Ping->ProcessEvent(SetPlayer, PlayerArgs.GetStructMemory());
	UCommonRichTextBlock* ConnectionText = Cast<UCommonRichTextBlock>(Ping->GetWidgetFromName(TEXT("ConnectionStatus")));
	if (!TestNotNull(TEXT("Production ping rich text"), ConnectionText))
	{
		return false;
	}
	{
		TGuardValue<FLinearColor> TextOverride(Palette->Shared.Text, Palette->Cyan.Primary);
		Invoke(Ping, TEXT("UpdateStatus"));
		TestTrue(TEXT("Ping markup reads the current shared text color"), ConnectionText->GetText().ToString().Contains(Palette->Shared.Text.ToFColorSRGB().ToHex()));
	}

	UClass* CueClass = LoadClass<AActor>(nullptr, TEXT("/Game/GameplayCueNotifies/GCNL_WaitingForPlayers.GCNL_WaitingForPlayers_C"));
	UClass* WaitingClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/Hud/GamePhase/W_WaitingForPlayers_Message.W_WaitingForPlayers_Message_C"));
	const FClassProperty* SpawnClass = CueClass ? FindFProperty<FClassProperty>(CueClass, TEXT("WidgetToSpawn")) : nullptr;
	if (!TestNotNull(TEXT("Waiting GameplayCue widget input"), SpawnClass) || !TestNotNull(TEXT("Waiting widget loads"), WaitingClass))
	{
		return false;
	}
	TestEqual(TEXT("Production waiting cue creates the migrated widget"), SpawnClass->GetObjectPropertyValue_InContainer(CueClass->GetDefaultObject()), static_cast<UObject*>(WaitingClass));
	UClass* PregameClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/Blueprints/Game/Phases/GE_PregameLobby.GE_PregameLobby_C"));
	const UGameplayEffect* Pregame = PregameClass ? PregameClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	bool bTriggersWaitingCue = false;
	if (TestNotNull(TEXT("Production pregame effect loads"), Pregame))
	{
		for (const FGameplayEffectCue& Cue : Pregame->GameplayCues)
		{
			bTriggersWaitingCue |= Cue.GameplayCueTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.UserMessage.WaitingForPlayers")));
		}
	}
	TestTrue(TEXT("Pregame effect triggers the waiting GameplayCue"), bTriggersWaitingCue);
	UUserWidget* Waiting = CreateWidget<UUserWidget>(Controller, WaitingClass);
	TSharedPtr<SWidget> WaitingSlate = Waiting->TakeWidget();
	ON_SCOPE_EXIT
	{
		Waiting->ReleaseSlateResources(true);
		WaitingSlate.Reset();
		Invoke(Ping, TEXT("CancelUIColorObserver"));
	};
	UCommonRichTextBlock* AssignmentText = Cast<UCommonRichTextBlock>(Waiting->GetWidgetFromName(TEXT("TeamNotification")));
	if (!TestNotNull(TEXT("Production assignment rich text"), AssignmentText))
	{
		return false;
	}
	auto CheckAssignment = [this, AssignmentText, Waiting](int32 TeamId, const TCHAR* TeamName)
	{
		const FString Text = AssignmentText->GetText().ToString();
		TestTrue(TEXT("Assignment markup reads the displayed team color"), Text.Contains(UArenaUIColorLibrary::GetTeamColor(TeamId, EArenaFactionColorRole::Primary).ToFColorSRGB().ToHex()));
		TestTrue(TEXT("Assignment markup reads shared text color"), Text.Contains(UArenaUIColorLibrary::GetSharedColor(EArenaSharedColorRole::Text).ToFColorSRGB().ToHex()));
		TestTrue(TEXT("Assignment name follows the same team"), Text.Contains(TeamName));
		UImage* Panel = Cast<UImage>(Waiting->GetWidgetFromName(TEXT("Border")));
		UMaterialInstanceDynamic* Material = Panel ? Panel->GetDynamicMaterial() : nullptr;
		if (TestNotNull(TEXT("Waiting panel material is available"), Material))
		{
			FLinearColor Value;
			Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Border_Inner_RGBA")), Value);
			TestEqual(TEXT("Waiting panel uses Surface rather than a large highlight"), Value.CopyWithNewOpacity(1.0f),
				UArenaUIColorLibrary::GetTeamColor(TeamId, EArenaFactionColorRole::Surface));
		}
	};
	CheckAssignment(1, TEXT("Red Team"));
	Player->SetGenericTeamId(FGenericTeamId(2));
	CheckAssignment(2, TEXT("Blue Team"));
	{
		TGuardValue<FLinearColor> PrimaryOverride(Palette->Cyan.Primary, Palette->Shared.TextMuted);
		TGuardValue<FLinearColor> TextOverride(Palette->Shared.Text, Palette->Orange.Highlight);
		Player->SetGenericTeamId(FGenericTeamId(1));
		Player->SetGenericTeamId(FGenericTeamId(2));
		CheckAssignment(2, TEXT("Blue Team"));
	}
	UClass* ReticleClass = LoadClass<UArenaReticleWidget>(nullptr, TEXT("/Game/UI/W_Reticle.W_Reticle_C"));
	if (!TestNotNull(TEXT("Production reticle loads"), ReticleClass))
	{
		return false;
	}
	for (EArenaHitFeedbackType Type : { EArenaHitFeedbackType::Hit, EArenaHitFeedbackType::Kill, EArenaHitFeedbackType::InstantKill })
	{
		UArenaReticleWidget* Reticle = CreateWidget<UArenaReticleWidget>(Controller, ReticleClass);
		TSharedPtr<SWidget> ReticleSlate = Reticle->TakeWidget();
		Reticle->ShowEnemyHit(Type);
		UImage* Feedback = Cast<UImage>(Reticle->GetWidgetFromName(TEXT("FeedbackFX")));
		UMaterialInstanceDynamic* Material = Feedback ? Feedback->GetDynamicMaterial() : nullptr;
		if (TestNotNull(TEXT("Feedback material is ready after the production event"), Material))
		{
			FLinearColor Value;
			Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Energy")), Value);
			TestEqual(TEXT("Every feedback type uses the owning player's primary"), Value, UArenaUIColorLibrary::GetTeamColor(2, EArenaFactionColorRole::Primary));
			Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("PaletteCore")), Value);
			TestEqual(TEXT("Feedback core reads the shared team palette"), Value, UArenaUIColorLibrary::GetTeamColor(2, EArenaFactionColorRole::Core));
			Player->SetGenericTeamId(FGenericTeamId(1));
			Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Energy")), Value);
			TestEqual(TEXT("Active feedback refreshes after a team change"), Value, UArenaUIColorLibrary::GetTeamColor(1, EArenaFactionColorRole::Primary));
			Player->SetGenericTeamId(FGenericTeamId(2));
		}
		Reticle->ReleaseSlateResources(true);
		ReticleSlate.Reset();
	}
	return true;
}
#endif
