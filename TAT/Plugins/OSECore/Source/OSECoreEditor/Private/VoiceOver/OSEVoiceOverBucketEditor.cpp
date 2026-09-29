// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT



#include "VoiceOver/OSEVoiceOverBucketEditor.h"

//ose
#include "OSEProjectSettings.h"

//ose editor
#include "VoiceOver/OSEVoiceOverEditorUtilities.h"

//ue4
#include "AkUnrealHelper.h"
#include "GameplayTagsManager.h"
#include "Platforms/AkUEPlatform.h"

DEFINE_LOG_CATEGORY_STATIC(LogOSEVoiceOverBucketEditor, Log, All);

#define LOCTEXT_NAMESPACE "AssetTypeActions"

TSharedRef<FOSEVoiceOverBucketEditor> FOSEVoiceOverBucketEditor::CreateEditor(const EToolkitMode::Type mode, const TSharedPtr<IToolkitHost> initToolkitHost, UOSEVoiceOverBucket* voiceBucket)
{
   TSharedRef<FOSEVoiceOverBucketEditor> newEditor = MakeShared<FOSEVoiceOverBucketEditor>();

   newEditor->InitVoiceLineEditor(mode, initToolkitHost, voiceBucket);

   return newEditor;
}

void FOSEVoiceOverBucketEditor::InitVoiceLineEditor(const EToolkitMode::Type mode, const TSharedPtr< class IToolkitHost >& initToolkitHost, UOSEVoiceOverBucket* voiceBucket)
{
   TArray<UObject*> objectsToEdit;
   objectsToEdit.Add(voiceBucket);
   _voiceBucket = voiceBucket;

   FSimpleAssetEditor::InitEditor(mode, initToolkitHost, objectsToEdit, FGetDetailsViewObjects());

   TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

   ToolbarExtender->AddToolBarExtension(
      "Asset",
      EExtensionHook::After,
      GetToolkitCommands(),
      FToolBarExtensionDelegate::CreateSP(this, &FOSEVoiceOverBucketEditor::_FillToolbar)
   );

   AddToolbarExtender(ToolbarExtender);
   RegenerateMenusAndToolbars();
}


void FOSEVoiceOverBucketEditor::_FillToolbar(FToolBarBuilder& toolbarBuilder)
{
   toolbarBuilder.BeginSection("Commands");
   {
   }
   toolbarBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE
