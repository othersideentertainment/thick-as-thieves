// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once
#include "Modules/ModuleManager.h"
#include "OSEGenericGraphEditorModule.h"
#include <IAssetTools.h>
#include <EdGraphUtilities.h>

class FOSEGenericGraphEditor : public IOSEGenericGraphEditor
{
   /** IModuleInterface implementation */
   virtual void StartupModule() override;
   virtual void ShutdownModule() override;


private:
   void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);
   void HandlePreSavePackage(UPackage* Package, FObjectPreSaveContext ObjectSaveContext);

private:
   TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

   EAssetTypeCategories::Type GenericGraphAssetCategoryBit;

   TSharedPtr<FGraphPanelNodeFactory> GraphPanelNodeFactory_GenericGraph;
};
