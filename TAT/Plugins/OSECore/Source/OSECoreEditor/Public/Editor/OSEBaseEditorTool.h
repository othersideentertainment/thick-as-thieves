// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "IDetailsView.h"
#include "DetailsViewArgs.h"

#include "OSEBaseEditorTool.generated.h"

UCLASS(Blueprintable, Abstract)
class OSECOREEDITOR_API UOSEBaseEditorTool : public UObject
{
   GENERATED_BODY()

public:
   // for our subclasses 
   virtual void InitEditorTool() { }
   virtual bool AllowMultipleWindows() const { return false; }
   virtual void OnEditorToolClosed() {}

   // Create the Slate widget for this tool.
   // The default implementation is a simple property editor for the tool object's properties.
   virtual TSharedRef<SWidget> CreateEditorToolWidget();

public:

   // base properties:

   // Name of the tool
   UPROPERTY(EditAnywhere, Category = "Hidden")
   FText ToolName;

   // Help text for the tool
   UPROPERTY(EditAnywhere, Category = "Hidden")
   FText ToolHelpText;

   UPROPERTY(EditAnywhere, Category = "Hidden")
   FVector2D ToolWindowInitialSize = FVector2D(400, 550);

   // statics:

   // Helpers for game-side menus to add this tool to their menu system
   static void AddToolToMenu(UClass* oseBaseEditorToolClass, FMenuBuilder& menuBuilder);
   static void AddToolToTab(UClass* oseBaseEditorToolClass, const TSharedRef<FTabManager>& tabManager, const TSharedRef<FWorkspaceItem>& group, const TOptional<FSlateIcon>& tabIcon = NullOpt);
   static void RemoveToolFromTab(UClass* oseBaseEditorToolClass, const TSharedRef<FTabManager>& tabManager);

protected:
   static TSharedRef<IDetailsView> CreatePropertyEditorWidget(UObject* editingObject, const TOptional<FDetailsViewArgs>& args = NullOpt);

private:
   static TSharedRef<SWindow> CreateFloatingWindow(const FText& title, const TSharedRef<SWidget>& contents, const FVector2D& initialSize);
   static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& spawnTabArgs, UClass* toolClass);
   static void TriggerTool(UClass* toolClass);
   static void OnWindowClosed(const TSharedRef<SWindow>& window, UOSEBaseEditorTool* toolObject);
   static void OnTabClosed(TSharedRef<SDockTab> tab, UOSEBaseEditorTool* toolObject);
   static TMap<UClass*, TSharedRef<SWindow>> _sOpenTools;
};
