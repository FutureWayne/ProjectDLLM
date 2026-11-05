// Copyright Ludens Studio. All Rights Reserved.

#include "UI/ArenaUIColorLibrary.h"

#include "Components/Image.h"
#include "Components/Border.h"
#include "Engine/CancellableAsyncAction.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Player/ArenaPlayerState.h"
#include "Settings/ArenaUIColorSettings.h"
#include "Tests/ArenaUIColorTestController.h"
#include "UI/Widget/ArenaHealthBarWidget.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaUIColorPaletteTest, "Arena.UI.Colors.Palette", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaUIColorPaletteTest::RunTest(const FString& Parameters)
{
	UArenaUIColorSettings* Settings = GetMutableDefault<UArenaUIColorSettings>();
	const UArenaUIColorPalette* Palette = Settings->GetPalette();
	if (!TestNotNull(TEXT("Configured UI palette loads"), Palette))
	{
		return false;
	}

	FDataValidationContext Context;
	TestTrue(TEXT("Configured palette passes validation"), Settings->IsDataValid(Context) == EDataValidationResult::Valid);
	TestTrue(TEXT("Palette load is retained"), Palette == Settings->GetPalette());

	// Read the authoritative table so tests do not introduce another editable color list.
	FString Rules;
	if (!TestTrue(TEXT("Load project color rules"), FFileHelper::LoadFileToString(Rules, *FPaths::Combine(FPaths::ProjectDir(), TEXT("Rule.md")))))
	{
		return false;
	}

	TArray<FString> Lines;
	Rules.ParseIntoArrayLines(Lines);
	int32 CheckedColors = 0;
	for (const FString& Line : Lines)
	{
		TArray<FString> Cells;
		Line.ParseIntoArray(Cells, TEXT("|"), true);
		if (Cells.Num() < 4)
		{
			continue;
		}

		for (FString& Cell : Cells)
		{
			Cell.TrimStartAndEndInline();
			Cell.ReplaceInline(TEXT("`"), TEXT(""));
		}

		if (Cells[0].StartsWith(TEXT("Shared.")))
		{
			const FString Name = Cells[0].RightChop(7);
			const int64 Role = StaticEnum<EArenaSharedColorRole>()->GetValueByNameString(Name);
			if (!TestTrue(*FString::Printf(TEXT("Known shared role %s"), *Name), Role != INDEX_NONE))
			{
				continue;
			}
			const FLinearColor Expected(FColor::FromHex(Cells[2]));
			TestTrue(*Cells[0], UArenaUIColorLibrary::GetSharedColor(static_cast<EArenaSharedColorRole>(Role)).Equals(Expected, 0.000001f));
			++CheckedColors;
		}
		else if (Cells.Num() >= 5 && Cells[2].StartsWith(TEXT("#")) && Cells[3].StartsWith(TEXT("#")))
		{
			const int64 Role = StaticEnum<EArenaFactionColorRole>()->GetValueByNameString(Cells[0]);
			if (!TestTrue(*FString::Printf(TEXT("Known faction role %s"), *Cells[0]), Role != INDEX_NONE))
			{
				continue;
			}
			const EArenaFactionColorRole ColorRole = static_cast<EArenaFactionColorRole>(Role);
			TestTrue(*FString::Printf(TEXT("Cyan %s"), *Cells[0]), UArenaUIColorLibrary::GetTeamColor(2, ColorRole).Equals(FLinearColor(FColor::FromHex(Cells[2])), 0.000001f));
			TestTrue(*FString::Printf(TEXT("Orange %s"), *Cells[0]), UArenaUIColorLibrary::GetTeamColor(1, ColorRole).Equals(FLinearColor(FColor::FromHex(Cells[3])), 0.000001f));
			CheckedColors += 2;
		}
	}
	TestEqual(TEXT("All twenty authoritative colors checked"), CheckedColors, 20);

	const FArenaSharedPalette Shared = UArenaUIColorLibrary::GetSharedPalette();
	const int32 NeutralTeamIds[] = { INDEX_NONE, 0, 3, 255 };
	for (int32 TeamId : NeutralTeamIds)
	{
		const FArenaFactionPalette Neutral = UArenaUIColorLibrary::GetTeamPalette(TeamId);
		TestEqual(TEXT("Neutral primary"), Neutral.Primary, Shared.Text);
		TestEqual(TEXT("Neutral highlight"), Neutral.Highlight, Shared.Text);
		TestEqual(TEXT("Neutral core"), Neutral.Core, Shared.Text);
		TestEqual(TEXT("Neutral secondary"), Neutral.Secondary, Shared.TextMuted);
		TestEqual(TEXT("Neutral muted"), Neutral.Muted, Shared.TextMuted);
		TestEqual(TEXT("Neutral edge"), Neutral.Edge, Shared.Panel);
		TestEqual(TEXT("Neutral surface"), Neutral.Surface, Shared.Panel);
		TestEqual(TEXT("Neutral deep"), Neutral.Deep, Shared.Background);
	}

	// Exercise the reflected entry point used by Blueprint, including output storage.
	UFunction* Function = UArenaUIColorLibrary::StaticClass()->FindFunctionByName(TEXT("GetTeamColor"));
	if (TestNotNull(TEXT("Blueprint color query exists"), Function))
	{
		TestTrue(TEXT("Query is BlueprintPure"), Function->HasAnyFunctionFlags(FUNC_BlueprintPure));
		FStructOnScope Args(Function);
		const FIntProperty* TeamProperty = FindFProperty<FIntProperty>(Function, TEXT("TeamId"));
		const FEnumProperty* RoleProperty = FindFProperty<FEnumProperty>(Function, TEXT("Role"));
		const FStructProperty* ReturnProperty = FindFProperty<FStructProperty>(Function, TEXT("ReturnValue"));
		if (!TestNotNull(TEXT("Team input"), TeamProperty) || !TestNotNull(TEXT("Role input"), RoleProperty) || !TestNotNull(TEXT("Color output"), ReturnProperty))
		{
			return false;
		}
		TeamProperty->SetPropertyValue_InContainer(Args.GetStructMemory(), 2);
		RoleProperty->GetUnderlyingProperty()->SetIntPropertyValue(RoleProperty->ContainerPtrToValuePtr<void>(Args.GetStructMemory()), static_cast<uint64>(EArenaFactionColorRole::Primary));
		GetMutableDefault<UArenaUIColorLibrary>()->ProcessEvent(Function, Args.GetStructMemory());
		TestEqual(TEXT("Blueprint and C++ return identical colors"), *ReturnProperty->ContainerPtrToValuePtr<FLinearColor>(Args.GetStructMemory()), UArenaUIColorLibrary::GetTeamColor(2, EArenaFactionColorRole::Primary));
	}

	UArenaUIColorPalette* EditablePalette = const_cast<UArenaUIColorPalette*>(Palette);
	{
		TGuardValue<FLinearColor> Override(EditablePalette->Cyan.Primary, Shared.Text);
		TestEqual(TEXT("Queries read shared asset values instead of cached color copies"), UArenaUIColorLibrary::GetTeamColor(2, EArenaFactionColorRole::Primary), Shared.Text);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaUIColorMissingPaletteTest, "Arena.UI.Colors.MissingPalette", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaUIColorMissingPaletteTest::RunTest(const FString& Parameters)
{
	UArenaUIColorSettings* Settings = GetMutableDefault<UArenaUIColorSettings>();
	{
		TGuardValue<TSoftObjectPtr<UArenaUIColorPalette>> Override(Settings->Palette, TSoftObjectPtr<UArenaUIColorPalette>());
		AddExpectedError(TEXT("Failed to load the shared UI palette"), EAutomationExpectedErrorFlags::Contains, 1);
		TestEqual(TEXT("Missing palette returns transparent"), UArenaUIColorLibrary::GetTeamColor(2, EArenaFactionColorRole::Primary), FLinearColor::Transparent);
		TestEqual(TEXT("Repeated query remains safe"), UArenaUIColorLibrary::GetSharedColor(EArenaSharedColorRole::Text), FLinearColor::Transparent);
		FDataValidationContext Context;
		TestTrue(TEXT("Missing palette fails validation"), Settings->IsDataValid(Context) == EDataValidationResult::Invalid);
	}
	TestNotNull(TEXT("Original configuration restored"), Settings->GetPalette());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaUIColorWidgetLifecycleTest, "Arena.UI.Colors.WidgetLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaUIColorWidgetLifecycleTest::RunTest(const FString& Parameters)
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
	UClass* WidgetClass = LoadClass<UArenaHealthBarWidget>(nullptr, TEXT("/Game/UI/Hud/HealthBar/W_Healthbar.W_Healthbar_C"));
	if (!TestNotNull(TEXT("Load production health bar"), WidgetClass))
	{
		return false;
	}

	UArenaHealthBarWidget* Widget = CreateWidget<UArenaHealthBarWidget>(Controller, WidgetClass);
	if (!TestNotNull(TEXT("Create production health bar"), Widget))
	{
		return false;
	}
	TSharedPtr<SWidget> SlateWidget = Widget->TakeWidget();
	UImage* Fill = Cast<UImage>(Widget->GetWidgetFromName(TEXT("BarFill")));
	if (!TestNotNull(TEXT("Production fill widget"), Fill))
	{
		return false;
	}
	UMaterialInstanceDynamic* Material = Fill->GetDynamicMaterial();
	auto CheckPrimary = [this, Material](const TCHAR* Label, int32 TeamId)
	{
		FLinearColor Value;
		TestTrue(TEXT("Material has Primary parameter"), Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Primary")), Value));
		TestEqual(Label, Value, UArenaUIColorLibrary::GetTeamColor(TeamId, EArenaFactionColorRole::Primary));
		TestTrue(TEXT("Material has Highlight parameter"), Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("UIColor_Own_Highlight")), Value));
		TestEqual(TEXT("Energy highlight follows the same displayed team"), Value, UArenaUIColorLibrary::GetTeamColor(TeamId, EArenaFactionColorRole::Highlight));
	};
	CheckPrimary(TEXT("Unassigned player uses neutral colors"), INDEX_NONE);

	// Simulate PlayerState replication through its production RepNotify, then use real team changes.
	AArenaPlayerState* PlayerState = World->SpawnActor<AArenaPlayerState>();
	PlayerState->SetGenericTeamId(FGenericTeamId(2));
	const FObjectPropertyBase* PlayerStateProperty = FindFProperty<FObjectPropertyBase>(AController::StaticClass(), TEXT("PlayerState"));
	UFunction* PlayerStateRepNotify = Controller->FindFunction(TEXT("OnRep_PlayerState"));
	if (!TestNotNull(TEXT("PlayerState property"), PlayerStateProperty) || !TestNotNull(TEXT("PlayerState RepNotify"), PlayerStateRepNotify))
	{
		return false;
	}
	PlayerStateProperty->SetObjectPropertyValue_InContainer(Controller, PlayerState);
	Controller->ProcessEvent(PlayerStateRepNotify, nullptr);
	CheckPrimary(TEXT("Late PlayerState assignment applies cyan"), 2);
	PlayerState->SetGenericTeamId(FGenericTeamId(1));
	CheckPrimary(TEXT("Team change applies orange to the same material instance"), 1);
	TestEqual(TEXT("Each widget retains its own MID"), Fill->GetDynamicMaterial(), Material);

	const FObjectPropertyBase* ObserverProperty = FindFProperty<FObjectPropertyBase>(WidgetClass, TEXT("UIColorObserver"));
	UCancellableAsyncAction* Observer = ObserverProperty ? Cast<UCancellableAsyncAction>(ObserverProperty->GetObjectPropertyValue_InContainer(Widget)) : nullptr;
	if (!TestNotNull(TEXT("WBP owns its team observer"), Observer))
	{
		return false;
	}
	Widget->ReleaseSlateResources(true);
	SlateWidget.Reset();
	TestFalse(TEXT("Destruction cancels team subscription"), Observer->IsActive());
	SlateWidget = Widget->TakeWidget();
	TestNotEqual(TEXT("Reconstruction creates a fresh observer"), ObserverProperty->GetObjectPropertyValue_InContainer(Widget), static_cast<UObject*>(Observer));
	PlayerState->SetGenericTeamId(FGenericTeamId(2));
	Material = Fill->GetDynamicMaterial();
	FLinearColor RebuiltColor;
	Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Primary")), RebuiltColor);
	TestEqual(TEXT("Reconstructed widget follows its current team"), RebuiltColor, UArenaUIColorLibrary::GetTeamColor(2, EArenaFactionColorRole::Primary));
	Widget->ReleaseSlateResources(true);
	SlateWidget.Reset();

	UArenaUIColorPalette* Palette = const_cast<UArenaUIColorPalette*>(GetMutableDefault<UArenaUIColorSettings>()->GetPalette());
	if (!TestNotNull(TEXT("Shared asset for recreation check"), Palette))
	{
		return false;
	}
	{
		const FArenaSharedPalette Shared = UArenaUIColorLibrary::GetSharedPalette();
		TGuardValue<FLinearColor> PrimaryOverride(Palette->Cyan.Primary, Shared.TextMuted);
		TGuardValue<FLinearColor> SurfaceOverride(Palette->Cyan.Surface, Shared.Panel);
		TGuardValue<FLinearColor> HighlightOverride(Palette->Cyan.Highlight, Shared.Text);
		UArenaHealthBarWidget* Reopened = CreateWidget<UArenaHealthBarWidget>(Controller, WidgetClass);
		if (!TestNotNull(TEXT("Reopen production health bar"), Reopened))
		{
			return false;
		}
		TSharedPtr<SWidget> ReopenedSlate = Reopened->TakeWidget();
		UImage* ReopenedFill = Cast<UImage>(Reopened->GetWidgetFromName(TEXT("BarFill")));
		UMaterialInstanceDynamic* ReopenedMaterial = ReopenedFill ? ReopenedFill->GetDynamicMaterial() : nullptr;
		if (TestNotNull(TEXT("Reopened fill material"), ReopenedMaterial))
		{
			FLinearColor Value;
			ReopenedMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Primary")), Value);
			TestEqual(TEXT("Reopening reads the changed primary from the shared asset"), Value, Shared.TextMuted);
			ReopenedMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("UIColor_Own_Surface")), Value);
			TestEqual(TEXT("Reopening reads the changed surface from the shared asset"), Value, Shared.Panel);
			ReopenedMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("UIColor_Own_Highlight")), Value);
			TestEqual(TEXT("Reopening reads the changed highlight from the shared asset"), Value, Shared.Text);
			TestNotEqual(TEXT("Separate widgets have separate material instances"), ReopenedMaterial, Material);
		}
		Reopened->ReleaseSlateResources(true);
		ReopenedSlate.Reset();
	}

	UClass* RowClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/Scoreboard/W_SB_PlayerRow_TDM.W_SB_PlayerRow_TDM_C"));
	UUserWidget* Row = RowClass ? CreateWidget<UUserWidget>(Controller, RowClass) : nullptr;
	if (!TestNotNull(TEXT("Create production scoreboard row"), Row))
	{
		return false;
	}
	UFunction* SetDisplayedPlayer = Row->FindFunction(TEXT("SetPlayerState"));
	const FObjectPropertyBase* DisplayedPlayerProperty = SetDisplayedPlayer ? FindFProperty<FObjectPropertyBase>(SetDisplayedPlayer, TEXT("PlayerState")) : nullptr;
	if (!TestNotNull(TEXT("Scoreboard display-object input"), DisplayedPlayerProperty))
	{
		return false;
	}
	auto DisplayPlayer = [Row, SetDisplayedPlayer, DisplayedPlayerProperty](AArenaPlayerState* State)
	{
		FStructOnScope Args(SetDisplayedPlayer);
		DisplayedPlayerProperty->SetObjectPropertyValue_InContainer(Args.GetStructMemory(), State);
		Row->ProcessEvent(SetDisplayedPlayer, Args.GetStructMemory());
	};
	AArenaPlayerState* FirstDisplayed = World->SpawnActor<AArenaPlayerState>();
	AArenaPlayerState* SecondDisplayed = World->SpawnActor<AArenaPlayerState>();
	FirstDisplayed->SetGenericTeamId(FGenericTeamId(1));
	SecondDisplayed->SetGenericTeamId(FGenericTeamId(2));
	DisplayPlayer(FirstDisplayed);
	TSharedPtr<SWidget> RowSlate = Row->TakeWidget();
	UBorder* EntryBorder = Cast<UBorder>(Row->GetWidgetFromName(TEXT("EntryBorder")));
	if (TestNotNull(TEXT("Scoreboard row background"), EntryBorder))
	{
		auto CheckRow = [this, EntryBorder](const TCHAR* Label, int32 TeamId)
		{
			TestEqual(Label, EntryBorder->GetBrushColor().CopyWithNewOpacity(1.0f), UArenaUIColorLibrary::GetTeamColor(TeamId, EArenaFactionColorRole::Deep));
		};
		CheckRow(TEXT("Row starts with displayed player's orange palette"), 1);
		DisplayPlayer(SecondDisplayed);
		CheckRow(TEXT("Row reuse switches to the new player's cyan palette"), 2);
		FirstDisplayed->SetGenericTeamId(FGenericTeamId(2));
		FirstDisplayed->SetGenericTeamId(FGenericTeamId(1));
		CheckRow(TEXT("Old displayed player's changes cannot recolor the reused row"), 2);
		SecondDisplayed->SetGenericTeamId(FGenericTeamId(1));
		CheckRow(TEXT("Reused row follows its current displayed player's team"), 1);
	}
	Row->ReleaseSlateResources(true);
	RowSlate.Reset();
	return true;
}
#endif
