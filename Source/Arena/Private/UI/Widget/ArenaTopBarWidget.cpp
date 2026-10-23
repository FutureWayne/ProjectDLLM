#include "UI/Widget/ArenaTopBarWidget.h"
#include "GameFramework/GameStateBase.h"
#include "Components/ActorComponent.h"
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
 if (!Source) return false;
 const FNumericProperty* Property = FindFProperty<FNumericProperty>(Source->GetClass(), Name);
 if (!Property) return false;
 const void* Data = Property->ContainerPtrToValuePtr<void>(Source);
 Value = Property->IsFloatingPoint() ? Property->GetFloatingPointPropertyValue(Data) : double(Property->GetSignedIntPropertyValue(Data));
 return FMath::IsFinite(Value);
}
}
UArenaTopBarWidget::UArenaTopBarWidget(const FObjectInitializer& Initializer) : Super(Initializer)
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
 ScoringSource.Reset(); bInitialized = false; bHasData = false;
 Super::NativeDestruct();
}
void UArenaTopBarWidget::RefreshSnapshot()
{
 AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
 if (!State) { ScoringSource.Reset(); bHasData = false; bInitialized = false; return; }
 if (!ScoringSource.IsValid() || ScoringSource->GetOuter() != State)
 {
  ScoringSource.Reset(); bInitialized = false;
  TInlineComponentArray<UActorComponent*> Components(State);
  for (UActorComponent* Component : Components)
  {
   if (Component->GetClass()->GetName().StartsWith(TEXT("B_ArenaDeathMatchScoring"))) { ScoringSource = Component; break; }
  }
 }
 double Target=0, Time=0;
 UArenaTeamSubsystem* Teams = GetWorld()->GetSubsystem<UArenaTeamSubsystem>();
 const FGameplayTag EliminationTag = FGameplayTag::RequestGameplayTag(TEXT("Score.Elimination"),false);
 bHasData = ReadTDMNumber(ScoringSource.Get(), TEXT("TargetScore"), Target)
  && ReadTDMNumber(ScoringSource.Get(), TEXT("CountdownTime"), Time) && Target > 0 && Teams && EliminationTag.IsValid();
 if (!bHasData) { bInitialized = false; return; }
 TargetKills = FMath::Clamp(int32(Target), 1, MAX_int32);
 Seconds = FMath::Max(0, FMath::CeilToInt(Time));
 // Match GetTeamScore: blue is team 2, red is team 1. Read replicated team tags,
 // not AttackerScore/DefenderScore caches which need not track each elimination.
 const int32 NewKills[2] = { FMath::Max(0,Teams->GetTeamTagStackCount(2,EliminationTag)), FMath::Max(0,Teams->GetTeamTagStackCount(1,EliminationTag)) };
 for (int32 Team=0; Team<2; ++Team)
 {
  if (bInitialized && NewKills[Team] != Kills[Team]) Pulse[Team] = 1;
  Kills[Team] = NewKills[Team];
  if (!bInitialized) { Progress[Team] = FMath::Clamp(float(Kills[Team])/TargetKills,0.f,1.f); Pulse[Team]=0; }
 }
 bInitialized = true;
}
void UArenaTopBarWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
 Super::NativeTick(Geometry, DeltaTime);
 AnimationClock += DeltaTime; SampleClock -= DeltaTime;
 if (SampleClock <= 0) { RefreshSnapshot(); SampleClock = .05f; }
 for (int32 Team=0; Team<2; ++Team)
 {
  Progress[Team] = FMath::FInterpTo(Progress[Team], TargetKills > 0 ? FMath::Clamp(float(Kills[Team])/TargetKills,0.f,1.f) : 0.f, DeltaTime, 7.f);
  Pulse[Team] = FMath::Max(0.f, Pulse[Team]-DeltaTime*1.8f);
 }
 Invalidate(EInvalidateWidgetReason::Paint);
}
int32 UArenaTopBarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Clip, FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const
{
 const float Scale = FMath::Min(G.GetLocalSize().X/1600.f, G.GetLocalSize().Y/112.f);
 if (Scale <= 0) return Layer;
 const FLinearColor Cyan(.025f,.72f,1.f), Amber(1.f,.57f,.08f), White(.78f,.94f,1.f);
 const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
 auto Box = [&](float X,float Y,float W,float H,FLinearColor Color) {
  FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(FVector2D(W*Scale,H*Scale),FSlateLayoutTransform(FVector2D(X*Scale,Y*Scale))),Brush,ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint());
 };
 auto Line = [&](TArray<FVector2D> Points,FLinearColor Color,float Width=1.f) {
  for (auto& P:Points) P*=Scale;
  FSlateDrawElement::MakeLines(Out,Layer+2,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint(),true,Width*Scale);
 };
 // Layered strokes retain a fine luminous core over both sky and dark geometry.
 auto GlowLine = [&](const TArray<FVector2D>& Points,FLinearColor Color,float Strength=1.f) {
  Line(Points,Color.CopyWithNewOpacity(.10f*Strength),6.f);
  Line(Points,Color.CopyWithNewOpacity(.24f*Strength),3.6f);
  Line(Points,Color.CopyWithNewOpacity(.5f*Strength),2.8f);
  const FLinearColor Core = FMath::Lerp(Color,White,.25f);
  Line(Points,Core.CopyWithNewOpacity(Strength),2.2f);
 };
 auto Text = [&](FString Value,float X,float Y,int32 Size,FLinearColor Color) {
  FSlateFontInfo Font = DisplayFont ? FSlateFontInfo(DisplayFont, FMath::Max(1,FMath::RoundToInt(Size*Scale))) : FCoreStyle::GetDefaultFontStyle("Regular",FMath::Max(1,FMath::RoundToInt(Size*Scale)));
  Font.OutlineSettings.OutlineSize = FMath::Max(1,FMath::RoundToInt(1.5f*Scale));
  Font.OutlineSettings.OutlineColor = FLinearColor(.002f,.008f,.016f,.95f);
  FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(FVector2D(500*Scale,100*Scale),FSlateLayoutTransform(FVector2D(X*Scale,Y*Scale))),Value,Font,ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint());
 };
 // Open reference silhouette: no score/timer backplates or enclosed frames.
 for(int32 Team=0;Team<2;++Team)
 {
  const FLinearColor Color=Team==0?Cyan:Amber;
  const float X=Team==0?20.f:1090.f, W=490.f, Fill=W*Progress[Team];
  const FLinearColor ContourColor = Team==0 ? FLinearColor(.02f,.6f,1.f) : FLinearColor(1.f,.30f,.025f);
  // Empty progress is transparent team-colored glass, never an opaque black strip.
  // Only the brighter moving beam below represents earned score.
  Box(X,40,W,18,ContourColor.CopyWithNewOpacity(.10f));
  GlowLine({{X,40},{X+W,40}},ContourColor,.7f);
  GlowLine({{X,58},{X+W,58}},ContourColor,.45f);
  for(int32 Glow=3;Glow>=1;--Glow) Box(Team==0?X:X+W-Fill,40-Glow,Fill,18+Glow*2,FLinearColor(Color.R,Color.G,Color.B,.035f*(4-Glow)));
  if(Fill>.2f)
  {
   const float Start=Team==0?X:X+W-Fill;
   Box(Start,42,Fill,14,Color.CopyWithNewOpacity(.8f));
   Box(Start,42,Fill,3,White.CopyWithNewOpacity(.8f));
   const float Scan=FMath::Fmod(AnimationClock*.27f,1.f)*Fill;
   Box(Team==0?Start+Scan:Start+Fill-Scan,43,2,12,White.CopyWithNewOpacity(.45f));
   Box(Team==0?X+Fill-2:X+W-Fill,39,2,20,White.CopyWithNewOpacity(.7f+Pulse[Team]*.3f));
  }
  GlowLine({{X,68},{X+W,68}},ContourColor,.55f);
  for(int32 Tick=0;Tick<=3;++Tick) GlowLine({{X+W*Tick/3,74},{X+W*Tick/3,84}},ContourColor,.8f);
  float Edge=Team==0?X-12:X+W+12, Dir=Team==0?1.f:-1.f;
  GlowLine({{Edge+Dir*10,32},{Edge,32},{Edge,70},{Edge+Dir*10,70}},ContourColor);
  const float SX=Team==0?535.f:945.f;
  // Author one contour on the blue side; mirror its geometry about the timer.
  // Text remains left-to-right on both teams.
  auto ScoreContour = [&](TArray<FVector2D> Points) {
   if (Team==1) for (FVector2D& Point:Points) Point.X=1600.f-Point.X;
   GlowLine(Points,ContourColor);
  };
  ScoreContour({{513,43},{527,30},{670,30},{692,51},{704,51}});
  ScoreContour({{704,67},{690,67},{678,80}});
  Text(Team==0?TEXT("TEAM BLUE"):TEXT("TEAM RED"),SX,3,17,White);
  const float Bounce=Pulse[Team]*FMath::Sin(Pulse[Team]*PI)*4;
  const FString KillText = bHasData ? FString::FromInt(Kills[Team]) : TEXT("--");
  const FString GoalText = bHasData ? FString::Printf(TEXT("/ %d"),TargetKills) : TEXT("/ --");
  const auto FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
  auto ScoreMetrics = [&](const FString& Value,int32 Size) {
   const int32 PixelSize=FMath::Max(1,FMath::RoundToInt(Size*Scale));
   const FSlateFontInfo Font=DisplayFont ? FSlateFontInfo(DisplayFont,PixelSize) : FCoreStyle::GetDefaultFontStyle("Regular",PixelSize);
   return FVector2D(FontMeasure->Measure(Value,Font).X/Scale,
    (FontMeasure->GetMaxCharacterHeight(Font)+FontMeasure->GetBaseline(Font))/Scale);
  };
  // Center the score pair in each mirrored frame and align both fonts to one baseline.
  const float ScoreCenter=Team==0 ? 598.5f : 1001.5f;
  const float Gap=10.f, Baseline=74.f;
  const FVector2D GoalMetrics=ScoreMetrics(GoalText,18);
  int32 KillSize=38;
  FVector2D KillMetrics=ScoreMetrics(KillText,KillSize);
  while(KillSize>22 && KillMetrics.X+Gap+GoalMetrics.X>143.f)
   KillMetrics=ScoreMetrics(KillText,--KillSize);
  const float ScoreX=ScoreCenter-(KillMetrics.X+Gap+GoalMetrics.X)*.5f;
  Text(KillText,ScoreX,Baseline-KillMetrics.Y-Bounce,KillSize,Color+(White-Color)*Pulse[Team]);
  Text(GoalText,ScoreX+KillMetrics.X+Gap,Baseline-GoalMetrics.Y,18,White);
  Text(TEXT("KILLS"),SX+3,86,14,White);
  if(Pulse[Team]>0) Line({{SX-7,83+Pulse[Team]*9},{SX+136,83+Pulse[Team]*9}},Color.CopyWithNewOpacity(Pulse[Team]));
 }
 const bool Warning=bHasData && Seconds<=30 && Seconds>0;
 const float Beat=Warning ? .5f+.5f*FMath::Sin(AnimationClock*(Seconds<=10?12.f:6.f)) : 0;
 const FLinearColor ClockColor=Warning?FLinearColor(1.f,.2f,.04f,1):White;
 const FString ClockText = bHasData ? FString::Printf(TEXT("%d:%02d"),Seconds/60,Seconds%60) : TEXT("--:--");
 const int32 ClockFontSize = FMath::Max(1,FMath::RoundToInt(44*Scale));
 const FSlateFontInfo ClockFont = DisplayFont ? FSlateFontInfo(DisplayFont,ClockFontSize) : FCoreStyle::GetDefaultFontStyle("Regular",ClockFontSize);
 const FVector2D ClockSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(ClockText,ClockFont) / Scale;
 Text(ClockText,800.f-ClockSize.X*.5f,56.f-ClockSize.Y*.5f,44,ClockColor);
 if(Warning)
 {
  Box(722,82,150,2,ClockColor.CopyWithNewOpacity(.3f+Beat*.7f));
  Line({{708,30},{702-Beat*4,48},{708,66}},ClockColor.CopyWithNewOpacity(.4f+Beat*.6f));
  Line({{888,30},{894+Beat*4,48},{888,66}},ClockColor.CopyWithNewOpacity(.4f+Beat*.6f));
 }
 return Layer+3;
}

