// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/AssetTypeActions/AssetTypeActions_OSEVoiceOverConversation.h"

//ose editor
#include "OSECoreEditor.h"
#include "VoiceOver/OSEVoiceOverConversationGraphEditor.h"
//ose
#include "VoiceOver/OSEVoiceOverConversation.h"

//ue4
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"

void FAssetTypeActions_OSEVoiceOverConversation::OpenAssetEditor(const TArray<UObject*>& objects, TSharedPtr<class IToolkitHost> editWithinLevelEditor)
{
   EToolkitMode::Type mode = editWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

   for (UObject* obj : objects)
   {
      if (UOSEVoiceOverConversation* conversation = Cast<UOSEVoiceOverConversation>(obj))
      {
         TSharedPtr<FOSEVoiceOverConversationGraphEditor> conversationEditor = MakeShared<FOSEVoiceOverConversationGraphEditor>();
         conversationEditor->Init(mode, editWithinLevelEditor, conversation);
      }
   }
}

UClass* FAssetTypeActions_OSEVoiceOverConversation::GetSupportedClass() const
{
   return UOSEVoiceOverConversation::StaticClass();
}

uint32 FAssetTypeActions_OSEVoiceOverConversation::GetCategories()
{
   FOSECoreEditor& oseCoreEditor = FModuleManager::LoadModuleChecked<FOSECoreEditor>("OSECoreEditor");
   return oseCoreEditor.GetOSEVoiceOverAssetCategoryBit();
}
