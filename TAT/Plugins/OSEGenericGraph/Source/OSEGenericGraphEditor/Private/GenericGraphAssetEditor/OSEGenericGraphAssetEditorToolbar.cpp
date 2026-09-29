// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/OSEGenericGraphAssetEditorToolbar.h"
#include "GenericGraphAssetEditor/OSEGenericGraphAssetEditor.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEditorCommands.h"
#include "GenericGraphAssetEditor/OSEGenericGraphEditorStyle.h"

#define LOCTEXT_NAMESPACE "AssetEditorToolbar_GenericGraph"

void FOSEGenericGraphAssetEditorToolbar::AddGenericGraphToolbar(TSharedPtr<FExtender> Extender)
{
   check(GenericGraphEditor.IsValid());
   TSharedPtr<FOSEGenericGraphAssetEditor> GenericGraphEditorPtr = GenericGraphEditor.Pin();

   TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
   ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, GenericGraphEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP( this, &FOSEGenericGraphAssetEditorToolbar::FillGenericGraphToolbar ));
   GenericGraphEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FOSEGenericGraphAssetEditorToolbar::FillGenericGraphToolbar(FToolBarBuilder& ToolbarBuilder)
{
   check(GenericGraphEditor.IsValid());
   TSharedPtr<FOSEGenericGraphAssetEditor> GenericGraphEditorPtr = GenericGraphEditor.Pin();

   ToolbarBuilder.BeginSection("Generic Graph");
   {
      ToolbarBuilder.AddToolBarButton(FOSEGenericGraphEditorCommands::Get().GraphSettings,
         NAME_None,
         LOCTEXT("GraphSettings_Label", "Graph Settings"),
         LOCTEXT("GraphSettings_ToolTip", "Show the Graph Settings"),
         FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings"));
   }
   ToolbarBuilder.EndSection();

   ToolbarBuilder.BeginSection("Util");
   {
      ToolbarBuilder.AddToolBarButton(FOSEGenericGraphEditorCommands::Get().Refresh,
         NAME_None,
         LOCTEXT("Refresh_Label", "Refresh"),
         LOCTEXT("Refresh_ToolTip", "Refreshes the graph, rebuilding all nodes"),
         FSlateIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh")));

      ToolbarBuilder.AddToolBarButton(FOSEGenericGraphEditorCommands::Get().AutoArrange,
         NAME_None,
         LOCTEXT("AutoArrange_Label", "Auto Arrange"),
         LOCTEXT("AutoArrange_ToolTip", "Auto arrange nodes, not perfect, but still handy"),
         FSlateIcon(FOSEGenericGraphEditorStyle::GetStyleSetName(), "OSEGenericGraphEditor.AutoArrange"));
   }
   ToolbarBuilder.EndSection();

}


#undef LOCTEXT_NAMESPACE
