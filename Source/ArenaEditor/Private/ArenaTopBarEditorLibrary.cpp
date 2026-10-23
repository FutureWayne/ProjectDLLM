#include "ArenaTopBarEditorLibrary.h"
#include "Components/Widget.h"
void UArenaTopBarEditorLibrary::DisableDesignerVariable(UWidget* Widget)
{
#if WITH_EDITORONLY_DATA
 if (Widget) { Widget->Modify(); Widget->bIsVariable = false; }
#endif
}
