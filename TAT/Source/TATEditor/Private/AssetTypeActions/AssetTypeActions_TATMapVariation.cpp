// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AssetTypeActions/AssetTypeActions_TATMapVariation.h"

//tat editor
#include "TATEditor.h"

//tat
#include "Variation/TATSpawnData.h"
#include "Variation/Clues/TATClueSet.h"
#include "Variation/Clues/TATCompoundClueSet.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Variation/SceneVariants/TATSceneSet.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Quests/TATMatchQuestDescription.h"

//ue4
#include "Editor.h"
#include "ToolMenus.h"


uint32 FAssetTypeActions_TATMapVariationBase::GetCategories()
{
   FTATEditor& tatEditor = FModuleManager::LoadModuleChecked<FTATEditor>("TATEditor");
   return tatEditor.GetTATMapVariationCategoryBit();
}

FText FAssetTypeActions_TATMapVariationBase::GetDisplayNameFromAssetData(const FAssetData& assetData) const
{
   return FText::GetEmpty();
}

UClass* FAssetTypeActions_TATSpawnData::GetSupportedClass() const
{
   return UTATSpawnDataAsset::StaticClass();
}

UClass* FAssetTypeActions_TATSceneSet::GetSupportedClass() const
{
   return UTATSceneSetAsset::StaticClass();
}

UClass* FAssetTypeActions_TATSceneAsset::GetSupportedClass() const
{
   return UTATSceneAsset::StaticClass();
}

UClass* FAssetTypeActions_TATSceneVariant::GetSupportedClass() const
{
   return UTATSceneVariantConfig::StaticClass();
}

UClass* FAssetTypeActions_TATClueSet::GetSupportedClass() const
{
   return UTATClueSet::StaticClass();
}

UClass* FAssetTypeActions_TATCompoundClueSet::GetSupportedClass() const
{
   return UTATCompoundClueSet::StaticClass();
}

UClass* FAssetTypeActions_TATMatchQuestDescription::GetSupportedClass() const
{
   return UTATMatchQuestDescription::StaticClass();
}
