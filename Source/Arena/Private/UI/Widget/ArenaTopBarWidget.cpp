// Copyright Ludens Studio. All Rights Reserved.

#include "UI/Widget/ArenaTopBarWidget.h"

#include "GameFramework/GameStateBase.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Rendering/SlateRenderer.h"
#include "Teams/ArenaTeamSubsystem.h"
#include "UObject/UnrealType.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	// Compatibility adapter for the existing Blueprint-owned replicated TDM state.
	// Resolve reflected names rather than loading a Blueprint class into the runtime module.
	bool ReadTDMNumber(const UObject* Source, FName Name, double& Value)
	{
		if (!Source)
		{
			return false;
		}

		const FNumericProperty* Property = FindFProperty<FNumericProperty>(Source->GetClass(), Name);
		if (!Property)
		{
			return false;
		}

		const void* Data = Property->ContainerPtrToValuePtr<void>(Source);
		Value = Property->IsFloatingPoint() ? Property->GetFloatingPointPropertyValue(Data) : double(Property->GetSignedIntPropertyValue(Data));
		return FMath::IsFinite(Value);
	}
} // namespace

UArenaTopBarWidget::UArenaTopBarWidget(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	DisplayFont = LoadObject<UObject>(nullptr, TEXT("/Game/UI/Foundation/Fonts/Orbitron.Orbitron"));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UArenaTopBarWidget::RebuildWidget()
{
	return SNew(SBox).WidthOverride(1600).HeightOverride(112);
}

void UArenaTopBarWidget::NativeDestruct()
{
	ScoringSource.Reset();
	bInitialized = false;
	bHasData = false;
	Super::NativeDestruct();
}

void UArenaTopBarWidget::RefreshSnapshot()
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		ScoringSource.Reset();
		bHasData = false;
		bInitialized = false;
		return;
	}

	if (!ScoringSource.IsValid() || ScoringSource->GetOuter() != GameState)
	{
		ScoringSource.Reset();
		bInitialized = false;
		TInlineComponentArray<UActorComponent*> Components(GameState);
		for (UActorComponent* Component : Components)
		{
			if (Component->GetClass()->GetName().StartsWith(TEXT("B_ArenaDeathMatchScoring")))
			{
				ScoringSource = Component;
				break;
			}
		}
	}

	double TargetScore = 0.0;
	double CountdownTime = 0.0;
	UArenaTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
	const FGameplayTag EliminationTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination"), false);
	bHasData = ReadTDMNumber(ScoringSource.Get(), TEXT("TargetScore"), TargetScore) && ReadTDMNumber(ScoringSource.Get(), TEXT("CountdownTime"), CountdownTime) && TargetScore > 0 && TeamSubsystem && EliminationTag.IsValid();
	if (!bHasData)
	{
		bInitialized = false;
		return;
	}

	TargetKills = FMath::Clamp(int32(TargetScore), 1, MAX_int32);
	Seconds = FMath::Max(0, FMath::CeilToInt(CountdownTime));
	// Match GetTeamScore: blue is team 2, red is team 1. Read replicated team tags,
	// not AttackerScore/DefenderScore caches which need not track each elimination.
	const int32 NewKills[2] = {FMath::Max(0, TeamSubsystem->GetTeamTagStackCount(2, EliminationTag)), FMath::Max(0, TeamSubsystem->GetTeamTagStackCount(1, EliminationTag))};
	for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
	{
		if (bInitialized && NewKills[TeamIndex] != Kills[TeamIndex])
		{
			Pulse[TeamIndex] = 1;
		}
		Kills[TeamIndex] = NewKills[TeamIndex];
		if (!bInitialized)
		{
			Progress[TeamIndex] = FMath::Clamp(float(Kills[TeamIndex]) / TargetKills, 0.f, 1.f);
			Pulse[TeamIndex] = 0;
		}
	}

	bInitialized = true;
}

void UArenaTopBarWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	AnimationClock += DeltaTime;
	SampleClock -= DeltaTime;

	if (SampleClock <= 0)
	{
		RefreshSnapshot();
		SampleClock = 0.05f;
	}

	for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
	{
		Progress[TeamIndex] = FMath::FInterpTo(Progress[TeamIndex], TargetKills > 0 ? FMath::Clamp(float(Kills[TeamIndex]) / TargetKills, 0.f, 1.f) : 0.f, DeltaTime, 7.f);
		Pulse[TeamIndex] = FMath::Max(0.f, Pulse[TeamIndex] - DeltaTime * 1.8f);
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UArenaTopBarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const float Scale = FMath::Min(Geometry.GetLocalSize().X / 1600.f, Geometry.GetLocalSize().Y / 112.f);
	if (Scale <= 0)
	{
		return Layer;
	}

	const FLinearColor Cyan(0.025f, 0.72f, 1.f);
	const FLinearColor Amber(1.f, 0.57f, 0.08f);
	const FLinearColor White(0.78f, 0.94f, 1.f);
	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");

	auto Box = [&](float X, float Y, float W, float H, FLinearColor Color)
	{
		FSlateDrawElement::MakeBox(Elements, Layer + 1, Geometry.ToPaintGeometry(FVector2D(W * Scale, H * Scale), FSlateLayoutTransform(FVector2D(X * Scale, Y * Scale))), Brush, ESlateDrawEffect::None, Color * Style.GetColorAndOpacityTint());
	};

	auto Line = [&](TArray<FVector2D> Points, FLinearColor Color, float Width = 1.f)
	{
		for (FVector2D& Point : Points)
		{
			Point *= Scale;
		}
		FSlateDrawElement::MakeLines(Elements, Layer + 2, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color * Style.GetColorAndOpacityTint(), true, Width * Scale);
	};

	// Layered strokes retain a fine luminous core over both sky and dark geometry.
	auto GlowLine = [&](const TArray<FVector2D>& Points, FLinearColor Color, float Strength = 1.f)
	{
		Line(Points, Color.CopyWithNewOpacity(0.10f * Strength), 6.f);
		Line(Points, Color.CopyWithNewOpacity(0.24f * Strength), 3.6f);
		Line(Points, Color.CopyWithNewOpacity(0.5f * Strength), 2.8f);
		const FLinearColor Core = FMath::Lerp(Color, White, 0.25f);
		Line(Points, Core.CopyWithNewOpacity(Strength), 2.2f);
	};

	auto Text = [&](FString Value, float X, float Y, int32 Size, FLinearColor Color)
	{
		FSlateFontInfo Font = DisplayFont ? FSlateFontInfo(DisplayFont, FMath::Max(1, FMath::RoundToInt(Size * Scale))) : FCoreStyle::GetDefaultFontStyle("Regular", FMath::Max(1, FMath::RoundToInt(Size * Scale)));
		Font.OutlineSettings.OutlineSize = FMath::Max(1, FMath::RoundToInt(1.5f * Scale));
		Font.OutlineSettings.OutlineColor = FLinearColor(0.002f, 0.008f, 0.016f, 0.95f);
		FSlateDrawElement::MakeText(Elements, Layer + 3, Geometry.ToPaintGeometry(FVector2D(500 * Scale, 100 * Scale), FSlateLayoutTransform(FVector2D(X * Scale, Y * Scale))), Value, Font, ESlateDrawEffect::None, Color * Style.GetColorAndOpacityTint());
	};

	// Open reference silhouette: no score/timer backplates or enclosed frames.
	for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
	{
		const FLinearColor Color = TeamIndex == 0 ? Cyan : Amber;
		const float X = TeamIndex == 0 ? 20.f : 1090.f;
		const float W = 490.f;
		const float Fill = W * Progress[TeamIndex];
		const FLinearColor ContourColor = TeamIndex == 0 ? FLinearColor(0.02f, 0.6f, 1.f) : FLinearColor(1.f, 0.30f, 0.025f);
		// Empty progress is transparent team-colored glass, never an opaque black strip.
		// Only the brighter moving beam below represents earned score.
		Box(X, 40, W, 18, ContourColor.CopyWithNewOpacity(0.10f));
		GlowLine({{X, 40}, {X + W, 40}}, ContourColor, 0.7f);
		GlowLine({{X, 58}, {X + W, 58}}, ContourColor, 0.45f);
		for (int32 Glow = 3; Glow >= 1; --Glow)
		{
			Box(TeamIndex == 0 ? X : X + W - Fill, 40 - Glow, Fill, 18 + Glow * 2, FLinearColor(Color.R, Color.G, Color.B, 0.035f * (4 - Glow)));
		}
		if (Fill > 0.2f)
		{
			const float Start = TeamIndex == 0 ? X : X + W - Fill;
			Box(Start, 42, Fill, 14, Color.CopyWithNewOpacity(0.8f));
			Box(Start, 42, Fill, 3, White.CopyWithNewOpacity(0.8f));
			const float Scan = FMath::Fmod(AnimationClock * 0.27f, 1.f) * Fill;
			Box(TeamIndex == 0 ? Start + Scan : Start + Fill - Scan, 43, 2, 12, White.CopyWithNewOpacity(0.45f));
			Box(TeamIndex == 0 ? X + Fill - 2 : X + W - Fill, 39, 2, 20, White.CopyWithNewOpacity(0.7f + Pulse[TeamIndex] * 0.3f));
		}
		GlowLine({{X, 68}, {X + W, 68}}, ContourColor, 0.55f);
		for (int32 Tick = 0; Tick <= 3; ++Tick)
		{
			GlowLine({{X + W * Tick / 3, 74}, {X + W * Tick / 3, 84}}, ContourColor, 0.8f);
		}
		const float Edge = TeamIndex == 0 ? X - 12 : X + W + 12;
		const float Direction = TeamIndex == 0 ? 1.f : -1.f;
		GlowLine({{Edge + Direction * 10, 32}, {Edge, 32}, {Edge, 70}, {Edge + Direction * 10, 70}}, ContourColor);
		const float ScoreLabelX = TeamIndex == 0 ? 535.f : 945.f;
		// Author one contour on the blue side; mirror its geometry about the timer.
		// Text remains left-to-right on both teams.
		auto ScoreContour = [&](TArray<FVector2D> Points)
		{
			if (TeamIndex == 1)
			{
				for (FVector2D& Point : Points)
				{
					Point.X = 1600.f - Point.X;
				}
			}
			GlowLine(Points, ContourColor);
		};

		ScoreContour({{513, 43}, {527, 30}, {670, 30}, {692, 51}, {704, 51}});
		ScoreContour({{704, 67}, {690, 67}, {678, 80}});
		Text(TeamIndex == 0 ? TEXT("TEAM BLUE") : TEXT("TEAM RED"), ScoreLabelX, 3, 17, White);
		const float Bounce = Pulse[TeamIndex] * FMath::Sin(Pulse[TeamIndex] * PI) * 4;
		const FString KillText = bHasData ? FString::FromInt(Kills[TeamIndex]) : TEXT("--");
		const FString GoalText = bHasData ? FString::Printf(TEXT("/ %d"), TargetKills) : TEXT("/ --");
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		auto ScoreMetrics = [&](const FString& Value, int32 Size)
		{
			const int32 PixelSize = FMath::Max(1, FMath::RoundToInt(Size * Scale));
			const FSlateFontInfo Font = DisplayFont ? FSlateFontInfo(DisplayFont, PixelSize) : FCoreStyle::GetDefaultFontStyle("Regular", PixelSize);
			return FVector2D(FontMeasure->Measure(Value, Font).X / Scale,
							 (FontMeasure->GetMaxCharacterHeight(Font) + FontMeasure->GetBaseline(Font)) / Scale);
		};

		// Center the score pair in each mirrored frame and align both fonts to one baseline.
		const float ScoreCenter = TeamIndex == 0 ? 598.5f : 1001.5f;
		const float Gap = 10.f;
		const float Baseline = 74.f;
		const FVector2D GoalMetrics = ScoreMetrics(GoalText, 18);
		int32 KillSize = 38;
		FVector2D KillMetrics = ScoreMetrics(KillText, KillSize);
		while (KillSize > 22 && KillMetrics.X + Gap + GoalMetrics.X > 143.f)
		{
			KillMetrics = ScoreMetrics(KillText, --KillSize);
		}
		const float ScoreX = ScoreCenter - (KillMetrics.X + Gap + GoalMetrics.X) * 0.5f;
		Text(KillText, ScoreX, Baseline - KillMetrics.Y - Bounce, KillSize, Color + (White - Color) * Pulse[TeamIndex]);
		Text(GoalText, ScoreX + KillMetrics.X + Gap, Baseline - GoalMetrics.Y, 18, White);
		Text(TEXT("KILLS"), ScoreLabelX + 3, 86, 14, White);
		if (Pulse[TeamIndex] > 0)
		{
			Line({{ScoreLabelX - 7, 83 + Pulse[TeamIndex] * 9}, {ScoreLabelX + 136, 83 + Pulse[TeamIndex] * 9}}, Color.CopyWithNewOpacity(Pulse[TeamIndex]));
		}
	}

	const bool bWarning = bHasData && Seconds <= 30 && Seconds > 0;
	const float Beat = bWarning ? 0.5f + 0.5f * FMath::Sin(AnimationClock * (Seconds <= 10 ? 12.f : 6.f)) : 0;
	const FLinearColor ClockColor = bWarning ? FLinearColor(1.f, 0.2f, 0.04f, 1) : White;
	const FString ClockText = bHasData ? FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60) : TEXT("--:--");
	const int32 ClockFontSize = FMath::Max(1, FMath::RoundToInt(44 * Scale));
	const FSlateFontInfo ClockFont = DisplayFont ? FSlateFontInfo(DisplayFont, ClockFontSize) : FCoreStyle::GetDefaultFontStyle("Regular", ClockFontSize);
	const FVector2D ClockSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(ClockText, ClockFont) / Scale;
	Text(ClockText, 800.f - ClockSize.X * 0.5f, 56.f - ClockSize.Y * 0.5f, 44, ClockColor);
	if (bWarning)
	{
		Box(722, 82, 150, 2, ClockColor.CopyWithNewOpacity(0.3f + Beat * 0.7f));
		Line({{708, 30}, {702 - Beat * 4, 48}, {708, 66}}, ClockColor.CopyWithNewOpacity(0.4f + Beat * 0.6f));
		Line({{888, 30}, {894 + Beat * 4, 48}, {888, 66}}, ClockColor.CopyWithNewOpacity(0.4f + Beat * 0.6f));
	}

	return Layer + 3;
}
