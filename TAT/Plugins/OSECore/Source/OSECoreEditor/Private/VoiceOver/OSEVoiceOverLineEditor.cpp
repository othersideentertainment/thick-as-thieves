// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT



#include "VoiceOver/OSEVoiceOverLineEditor.h"

//ose
#include "OSEProjectSettings.h"

//ose editor
#include "VoiceOver/OSEVoiceOverEditorUtilities.h"

//ue4
#include "AkUnrealHelper.h"
#include "GameplayTagsManager.h"
#include "Platforms/AkUEPlatform.h"

DEFINE_LOG_CATEGORY_STATIC(LogOSEVoiceOverLineEditor, Log, All);

#define LOCTEXT_NAMESPACE "AssetTypeActions"

TSharedRef<FOSEVoiceOverLineEditor> FOSEVoiceOverLineEditor::CreateEditor(const EToolkitMode::Type mode, const TSharedPtr<IToolkitHost> initToolkitHost, UOSEVoiceOverLine* voiceLine)
{
   TSharedRef<FOSEVoiceOverLineEditor> newEditor = MakeShared<FOSEVoiceOverLineEditor>();

   newEditor->InitVoiceLineEditor(mode, initToolkitHost, voiceLine);

   return newEditor;
}

void FOSEVoiceOverLineEditor::InitVoiceLineEditor(const EToolkitMode::Type mode, const TSharedPtr< class IToolkitHost >& initToolkitHost, UOSEVoiceOverLine* voiceLine)
{
   TArray<UObject*> objectsToEdit;
   objectsToEdit.Add(voiceLine);
   _voiceLine = voiceLine;

   FSimpleAssetEditor::InitEditor(mode, initToolkitHost, objectsToEdit, FGetDetailsViewObjects());

   TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);


   ToolbarExtender->AddToolBarExtension(
      "Asset",
      EExtensionHook::After,
      GetToolkitCommands(),
      FToolBarExtensionDelegate::CreateSP(this, &FOSEVoiceOverLineEditor::_FillToolbar)
   );

   AddToolbarExtender(ToolbarExtender);
   RegenerateMenusAndToolbars();
}


void FOSEVoiceOverLineEditor::_FillToolbar(FToolBarBuilder& toolbarBuilder)
{
   toolbarBuilder.BeginSection("Commands");
   {
      toolbarBuilder.AddToolBarButton(FExecuteAction::CreateSP(this, &FOSEVoiceOverLineEditor::_OnAutoPopulateClicked), NAME_None, LOCTEXT("OSEVoiceOverLineCommands_Populate", "Populate"));
   }
   toolbarBuilder.EndSection();
}

void FOSEVoiceOverLineEditor::_OnAutoPopulateClicked()
{
   const FScopedTransaction Transaction(LOCTEXT("OSEVoiceOverLineEditorTransacion_AutoPopulate", "Auto Populate Voice Line"));
   FOSEVoiceOverEditorUtilities::PopulateVoiceLine(_voiceLine);
}

#undef LOCTEXT_NAMESPACE
