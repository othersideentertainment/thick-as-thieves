// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Editor/OSEBaseEditorTool.h"

// ue5
#include "LevelEditor.h"
#include "PropertyEditorModule.h"
#include "Interfaces/IMainFrameModule.h"
#include "Widgets/Docking/SDockTab.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEBaseEditorTool)

TMap<UClass*, TSharedRef<SWindow>> UOSEBaseEditorTool::_sOpenTools;

TSharedRef<SWidget> UOSEBaseEditorTool::CreateEditorToolWidget()
{
   FPropertyEditorModule& propertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
   FDetailsViewArgs args;
   args.bHideSelectionTip = true;
   TSharedRef<IDetailsView> detailView = propertyEditorModule.CreateDetailView(args);
   detailView->SetObject(this);
   return detailView;
}

/* static */
void UOSEBaseEditorTool::AddToolToMenu(UClass* oseBaseEditorToolClass, FMenuBuilder& menuBuilder)
{
   check(oseBaseEditorToolClass && oseBaseEditorToolClass->IsChildOf(UOSEBaseEditorTool::StaticClass()));

   UOSEBaseEditorTool* cdo = oseBaseEditorToolClass->GetDefaultObject<UOSEBaseEditorTool>();

   menuBuilder.AddMenuEntry(
      cdo->ToolName,
      cdo->ToolHelpText,
      FSlateIcon(),
      FUIAction(FExecuteAction::CreateStatic(&UOSEBaseEditorTool::TriggerTool, oseBaseEditorToolClass)));
}

// static
void UOSEBaseEditorTool::AddToolToTab(UClass* oseBaseEditorToolClass, const TSharedRef<FTabManager>& tabManager, const TSharedRef<FWorkspaceItem>& group, const TOptional<FSlateIcon>& tabIcon)
{
   check(oseBaseEditorToolClass && oseBaseEditorToolClass->IsChildOf(UOSEBaseEditorTool::StaticClass()));
   UOSEBaseEditorTool* cdo = oseBaseEditorToolClass->GetDefaultObject<UOSEBaseEditorTool>();
   check(cdo != nullptr);
   const FName registrationName = oseBaseEditorToolClass->GetFName();
   if (!tabManager->HasTabSpawner(registrationName))
   {
      FTabSpawnerEntry& spawner = tabManager->RegisterTabSpawner(registrationName, FOnSpawnTab::CreateStatic(&UOSEBaseEditorTool::SpawnTab, oseBaseEditorToolClass))
         .SetDisplayName(cdo->ToolName)
         .SetTooltipText(cdo->ToolHelpText)
         .SetGroup(group);
      if (tabIcon)
      {
         spawner.SetIcon(*tabIcon);
      }
   }
}

// static
void UOSEBaseEditorTool::RemoveToolFromTab(UClass* oseBaseEditorToolClass, const TSharedRef<FTabManager>& tabManager)
{
   check(oseBaseEditorToolClass && oseBaseEditorToolClass->IsChildOf(UOSEBaseEditorTool::StaticClass()));
   tabManager->UnregisterTabSpawner(oseBaseEditorToolClass->GetFName());
}

// static
TSharedRef<IDetailsView> UOSEBaseEditorTool::CreatePropertyEditorWidget(UObject* editingObject, const TOptional<FDetailsViewArgs>& args)
{
   FPropertyEditorModule& propertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
   FDetailsViewArgs viewArgs;
   if (args)
   {
      viewArgs = *args;
   }
   else
   {
      viewArgs.bHideSelectionTip = true;
   }
   TSharedRef<IDetailsView> detailView = propertyEditorModule.CreateDetailView(viewArgs);
   detailView->SetObject(editingObject);
   return detailView;
}

// static
TSharedRef<SWindow> UOSEBaseEditorTool::CreateFloatingWindow(const FText& title, const TSharedRef<SWidget>& contents, const FVector2D& initialSize)
{
   TSharedRef<SWindow> newSlateWindow = SNew(SWindow)
      .Title(title)
      .ClientSize(initialSize);

   // If the main frame exists parent the window to it
   TSharedPtr<SWindow> parentWindow;
   if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
   {
      parentWindow = FModuleManager::GetModuleChecked<IMainFrameModule>("MainFrame").GetParentWindow();
   }

   if (parentWindow.IsValid())
   {
      FSlateApplication::Get().AddWindowAsNativeChild(newSlateWindow, parentWindow.ToSharedRef());
   }
   else
   {
      FSlateApplication::Get().AddWindow(newSlateWindow);
   }

   newSlateWindow->SetContent(
      SNew(SBorder)
      .BorderImage(FAppStyle::GetBrush(TEXT("PropertyWindow.WindowBorder")))
      [
         contents
      ]
   );

   return newSlateWindow;
}

TSharedRef<SDockTab> UOSEBaseEditorTool::SpawnTab(const FSpawnTabArgs& spawnTabArgs, UClass* toolClass)
{
   UOSEBaseEditorTool* toolObject = NewObject<UOSEBaseEditorTool>(GetTransientPackage(), toolClass);
   toolObject->InitEditorTool();
   toolObject->AddToRoot(); // don't gc

   return SNew(SDockTab)
      .OnTabClosed_Static(&UOSEBaseEditorTool::OnTabClosed, toolObject)
      [
         SNew(SBorder)
         .Padding(0.0f)
         .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
         [
            toolObject->CreateEditorToolWidget()
         ]
      ];
}

/* static */
void UOSEBaseEditorTool::TriggerTool(UClass* toolClass)
{
   const bool openInNewWindow = toolClass->GetDefaultObject<UOSEBaseEditorTool>()->AllowMultipleWindows();
   if (openInNewWindow || !_sOpenTools.Contains(toolClass))
   {
      UOSEBaseEditorTool* toolObject = NewObject<UOSEBaseEditorTool>(GetTransientPackage(), toolClass);
      toolObject->InitEditorTool();
      toolObject->AddToRoot(); // don't gc

      TSharedRef<SWindow> window = CreateFloatingWindow(toolObject->ToolName, toolObject->CreateEditorToolWidget(), toolObject->ToolWindowInitialSize);

      window->SetOnWindowClosed(FOnWindowClosed::CreateStatic(&UOSEBaseEditorTool::OnWindowClosed, toolObject));

      _sOpenTools.Add(toolClass, window);
   }
   else if (_sOpenTools.Contains(toolClass))
   {
      TSharedRef<SWindow> window = _sOpenTools[toolClass];
      window->BringToFront();
   }
}

/* static */
void UOSEBaseEditorTool::OnWindowClosed(const TSharedRef<SWindow>& window, UOSEBaseEditorTool* toolObject)
{
   check(toolObject != nullptr);

   toolObject->OnEditorToolClosed();

   // cleanup
   _sOpenTools.Remove(toolObject->GetClass());

   // allow gc
   toolObject->RemoveFromRoot();
}

void UOSEBaseEditorTool::OnTabClosed(TSharedRef<SDockTab> tab, UOSEBaseEditorTool* toolObject)
{
   check(toolObject != nullptr);

   toolObject->OnEditorToolClosed();

   // allow gc
   toolObject->RemoveFromRoot();
}
