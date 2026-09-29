// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AssetTypeActions/AssetTypeActions_TATUpgrades.h"

// tat editor
#include "TATEditor.h"

// tat
#include "Upgrades/TATUpgradeType.h"
#include "Upgrades/TATUpgradeGraph.h"

// ue
#include "Editor.h"
#include "ToolMenus.h"

UClass* FAssetTypeActions_TATUpgradeType::GetSupportedClass() const
{
   return UTATUpgradeType::StaticClass();
}

uint32 FAssetTypeActions_TATUpgradeType::GetCategories()
{
   FTATEditor& tatEditor = FModuleManager::LoadModuleChecked<FTATEditor>("TATEditor");
   return tatEditor.GetTATUpgradeCategoryBit();
}

FAssetTypeActions_TATUpgradeGraph::FAssetTypeActions_TATUpgradeGraph()
   : FOSEGenericGraphAssetTypeActions(FModuleManager::LoadModuleChecked<FTATEditor>("TATEditor").GetTATUpgradeCategoryBit())
{
}

UClass* FAssetTypeActions_TATUpgradeGraph::GetSupportedClass() const
{
   return UTATUpgradeGraph::StaticClass();
}
