// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Items/Tokens/TATInventoryToken.h"

// ue
#include "AssetDefinition_DataAsset.h"

#include "AssetDefinition_Tokens.generated.h"

UCLASS()
class TATEDITOR_API UAssetDefinition_TATInventoryToken : public UAssetDefinition_DataAsset
{
   GENERATED_BODY()
public:
   // UAssetDefinition Begin
   virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_InventoryToken", "TAT Inventory Token"); }
   virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(175, 0, 255)); }
   virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UTATInventoryToken::StaticClass(); }
   virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
   {
      static const FAssetCategoryPath kCategories[] = { FAssetCategoryPath(NSLOCTEXT("AssetTypeActions", "TAT Inventory Submenu", "TAT Inventory")) };
      return kCategories;
   }
   // UAssetDefinition End
};
