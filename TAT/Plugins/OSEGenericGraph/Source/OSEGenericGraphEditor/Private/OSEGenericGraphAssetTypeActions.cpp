// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "OSEGenericGraphAssetTypeActions.h"
#include "OSEGenericGraphEditorPCH.h"
#include "GenericGraphAssetEditor/OSEGenericGraphAssetEditor.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphAssetTypeActions"

FOSEGenericGraphAssetTypeActions::FOSEGenericGraphAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
   : MyAssetCategory(InAssetCategory)
{
}

FText FOSEGenericGraphAssetTypeActions::GetName() const
{
   return LOCTEXT("FGenericGraphAssetTypeActionsName", "Generic Graph");
}

FColor FOSEGenericGraphAssetTypeActions::GetTypeColor() const
{
   return FColor(129, 196, 115);
}

UClass* FOSEGenericGraphAssetTypeActions::GetSupportedClass() const
{
   return UOSEGenericGraph::StaticClass();
}

void FOSEGenericGraphAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
   const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

   for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
   {
      if (UOSEGenericGraph* Graph = Cast<UOSEGenericGraph>(*ObjIt))
      {
         TSharedRef<FOSEGenericGraphAssetEditor> NewGraphEditor(new FOSEGenericGraphAssetEditor());
         NewGraphEditor->InitGenericGraphAssetEditor(Mode, EditWithinLevelEditor, Graph);
      }
   }
}

uint32 FOSEGenericGraphAssetTypeActions::GetCategories()
{
   return MyAssetCategory;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
