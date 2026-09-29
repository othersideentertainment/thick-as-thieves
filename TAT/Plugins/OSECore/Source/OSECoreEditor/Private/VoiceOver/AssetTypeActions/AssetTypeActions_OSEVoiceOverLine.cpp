// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/AssetTypeActions/AssetTypeActions_OSEVoiceOverLine.h"

//ose editor
#include "OSECoreEditor.h"
#include "VoiceOver/OSEVoiceOverLineEditor.h"

//ose
#include "VoiceOver/OSEVoiceOverLine.h"

//ue4
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"



UClass* FAssetTypeActions_OSEVoiceOverLine::GetSupportedClass() const
{
   return UOSEVoiceOverLine::StaticClass();
}

uint32 FAssetTypeActions_OSEVoiceOverLine::GetCategories()
{
   FOSECoreEditor& oseCoreEditor = FModuleManager::LoadModuleChecked<FOSECoreEditor>("OSECoreEditor");
   return oseCoreEditor.GetOSEVoiceOverAssetCategoryBit();
}

void FAssetTypeActions_OSEVoiceOverLine::OpenAssetEditor(const TArray<UObject*>& objects, TSharedPtr<class IToolkitHost> editWithinLevelEditor)
{
   const EToolkitMode::Type mode = editWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

   for (auto ObjIt = objects.CreateConstIterator(); ObjIt; ++ObjIt)
   {
      if (UOSEVoiceOverLine* voiceLine = Cast<UOSEVoiceOverLine>(*ObjIt))
      {
         FOSEVoiceOverLineEditor::CreateEditor(mode, editWithinLevelEditor, voiceLine);
      }
   }
}
