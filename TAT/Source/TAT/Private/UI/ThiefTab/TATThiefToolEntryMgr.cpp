// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/ThiefTab/TATThiefToolEntryMgr.h"

// tat
#include "UI/ThiefTab/TATThiefToolEntryWidget.h"

// ose
#include "Items/ToolComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefToolEntryMgr)
DEFINE_LOG_CATEGORY_STATIC(LogTATThiefToolEntryMgr, Log, All);


UTATThiefToolEntryWidget* FTATThiefToolEntryMgr::ConstructToolEntryWidget(const UToolComponent* toolComponent, TSubclassOf<UTATThiefToolEntryWidget> toolEntryWidgetClass, UWidget* ownerWidget)
{
   check(IsValid(ownerWidget));

   UTATThiefToolEntryWidget* toolEntryWidget = nullptr;
   if (IsValid(toolEntryWidgetClass))
   {
      toolEntryWidget = CreateWidget<UTATThiefToolEntryWidget>(ownerWidget, toolEntryWidgetClass);
   }
   else
   {
      UE_LOG(LogTATThiefToolEntryMgr, Warning, TEXT("ConstructToolEntryWidget() called with invalid toolEntryWidgetClass! Constructing a native UTATThiefToolEntryWidget instance..."));
      toolEntryWidget = CreateWidget<UTATThiefToolEntryWidget>(ownerWidget);
   }

   check(IsValid(toolComponent));
   toolEntryWidget->SetTool(toolComponent);
   _toolEntryWidgets.Add(toolEntryWidget);
   return toolEntryWidget;
}

UTATThiefToolEntryWidget* FTATThiefToolEntryMgr::GetWidgetForTool(const UToolComponent* toolComponent) const
{
   // Deliberately avoid usage of IsValid(), as this can be called by OnToolRemoved which often provides callbacks on client with pending-kill tools
   check(toolComponent);
   for (UTATThiefToolEntryWidget* toolEntryWidget : _toolEntryWidgets)
   {
      if (toolComponent == toolEntryWidget->GetTool())
      {
         return toolEntryWidget;
      }
   }

   UE_LOG(LogTATThiefToolEntryMgr, Warning, TEXT("GetWidgetForTool() | could not find widget associated with tool %s!"), *toolComponent->GetName());
   return nullptr;
}

void FTATThiefToolEntryMgr::RemoveToolEntryWidget(UTATThiefToolEntryWidget* toolEntryWidget)
{
   check(IsValid(toolEntryWidget));
   ensure(_toolEntryWidgets.Remove(toolEntryWidget) > 0);
}

void FTATThiefToolEntryMgr::ClearToolEntryWidgets()
{
   _toolEntryWidgets.Reset();
}
