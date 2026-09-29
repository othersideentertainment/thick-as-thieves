// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/DataTableValidator_Loot.h"

// tat
#include "Loot/TATLootTypes.h"

// ue5
#include "Engine/DataTable.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DataTableValidator_Loot)


bool UDataTableValidator_Loot::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   const UDataTable* dataTable = Cast<UDataTable>(asset);
   return dataTable && dataTable->GetRowStruct()->IsChildOf(FTATLootInfo::StaticStruct());
}

EDataValidationResult UDataTableValidator_Loot::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   const UDataTable* dataTable = CastChecked<UDataTable>(asset);

   auto reportError = [this, asset](const FText& message) {
         AssetFails(asset, message);
      };

   dataTable->ForeachRow<FTATLootInfo>(TEXT("Validator"), [reportError](const FName& key, const FTATLootInfo& loot)
      {
         auto reportScoped = [&reportError, key](const FText& message) {
            reportError(FText::FormatOrdered(INVTEXT("[{0}] {1}"), FText::FromString(key.ToString()), message));
         };

         if (!loot.LootIdentifier.IsValid())
         {
            reportScoped(INVTEXT("No LootIdentifier"));
         }
         if (loot.DisplayName.IsEmpty())
         {
            reportScoped(INVTEXT("No display name"));
         }
         if (loot.DisplaySprite.IsNull())
         {
            reportScoped(INVTEXT("No display sprite"));
         }
         if (loot.ActorClass.IsNull())
         {
            reportScoped(INVTEXT("No ActorClass"));
         }

         if (loot.IsLargeCarry && loot.CarriedLootMeshData.ToolMesh.IsNull())
         {
            reportScoped(INVTEXT("Large carry loot has no CarriedLootMeshData"));
         }
      });

   if (!IsValidationStateSet())
   {
      AssetPasses(asset);
   }
   return GetValidationResult();
}
