// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/AssetTypeActions/AssetTypeActions_OSEVoiceOverBucket.h"

//ose editor
#include "OSECoreEditor.h"
#include "VoiceOver/OSEVoiceOverBucketEditor.h"

//ose
#include "VoiceOver/OSEVoiceOverBucket.h"

//ue4
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"



UClass* FAssetTypeActions_OSEVoiceOverBucket::GetSupportedClass() const
{
   return UOSEVoiceOverBucket::StaticClass();
}

uint32 FAssetTypeActions_OSEVoiceOverBucket::GetCategories()
{
   FOSECoreEditor& oseCoreEditor = FModuleManager::LoadModuleChecked<FOSECoreEditor>("OSECoreEditor");
   return oseCoreEditor.GetOSEVoiceOverAssetCategoryBit();
}

void FAssetTypeActions_OSEVoiceOverBucket::OpenAssetEditor(const TArray<UObject*>& objects, TSharedPtr<class IToolkitHost> editWithinLevelEditor)
{
   const EToolkitMode::Type mode = editWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

   for (auto ObjIt = objects.CreateConstIterator(); ObjIt; ++ObjIt)
   {
      if (UOSEVoiceOverBucket* voiceBucket = Cast<UOSEVoiceOverBucket>(*ObjIt))
      {
         FOSEVoiceOverBucketEditor::CreateEditor(mode, editWithinLevelEditor, voiceBucket);
      }
   }
}
