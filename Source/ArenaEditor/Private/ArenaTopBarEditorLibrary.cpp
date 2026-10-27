#include "ArenaTopBarEditorLibrary.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MovieScene.h"
#include "WidgetBlueprint.h"
void UArenaTopBarEditorLibrary::DisableDesignerVariable(UWidget* Widget)
{
 SetDesignerVariable(Widget, false);
}

void UArenaTopBarEditorLibrary::SetDesignerVariable(UWidget* Widget, bool bIsVariable)
{
#if WITH_EDITORONLY_DATA
 if (Widget) { Widget->Modify(); Widget->bIsVariable = bIsVariable; }
#endif
}

bool UArenaTopBarEditorLibrary::SetWidgetBlueprintRoot(UWidgetBlueprint* Blueprint, UWidget* Root)
{
 if (!Blueprint || !Blueprint->WidgetTree || !Root || Root->GetOuter() != Blueprint->WidgetTree)
 {
  return false;
 }
 Blueprint->Modify();
 Blueprint->WidgetTree->Modify();
 Blueprint->WidgetTree->RootWidget = Root;
 return true;
}

UWidgetAnimation* UArenaTopBarEditorLibrary::FindOrAddWidgetAnimation(UWidgetBlueprint* Blueprint, FName Name, float DurationSeconds)
{
 if (!Blueprint || Name.IsNone())
 {
  return nullptr;
 }
 for (UWidgetAnimation* Existing : Blueprint->Animations)
 {
  if (Existing && Existing->GetFName() == Name)
  {
   return Existing;
  }
 }

 Blueprint->Modify();
 UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(Blueprint, Name, RF_Transactional);
 Animation->SetDisplayLabel(Name.ToString());
 Animation->MovieScene = NewObject<UMovieScene>(Animation, Name, RF_Transactional);
 Animation->MovieScene->SetDisplayRate(FFrameRate(60, 1));
 const FFrameNumber End = (FMath::Max(DurationSeconds, 0.01f) * Animation->MovieScene->GetTickResolution()).RoundToFrame();
 Animation->MovieScene->SetPlaybackRange(TRange<FFrameNumber>(FFrameNumber(0), End + 1));
 Animation->MovieScene->GetEditorData().WorkStart = 0.0;
 Animation->MovieScene->GetEditorData().WorkEnd = DurationSeconds;
 Blueprint->Animations.Add(Animation);
 FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
 return Animation;
}

FGuid UArenaTopBarEditorLibrary::BindWidgetToAnimation(UWidgetAnimation* Animation, UWidget* Widget)
{
 if (!Animation || !Animation->MovieScene || !Widget)
 {
  return FGuid();
 }
 for (const FWidgetAnimationBinding& Binding : Animation->GetBindings())
 {
  if (Binding.WidgetName == Widget->GetFName() && Binding.SlotWidgetName.IsNone())
  {
   return Binding.AnimationGuid;
  }
 }

 Animation->Modify();
 Animation->MovieScene->Modify();
 const FGuid Guid = Animation->MovieScene->AddPossessable(Widget->GetName(), Widget->GetClass());
 FWidgetAnimationBinding Binding;
 Binding.AnimationGuid = Guid;
 Binding.WidgetName = Widget->GetFName();
 Binding.bIsRootWidget = false;
 Animation->AnimationBindings.Add(Binding);
 return Guid;
}
