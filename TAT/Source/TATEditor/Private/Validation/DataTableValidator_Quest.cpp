// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/DataTableValidator_Quest.h"

// tat
#include "Quests/TATQuestInfo.h"

// ue5
#include "Engine/DataTable.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DataTableValidator_Quest)


bool UDataTableValidator_Quest::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   const UDataTable* dataTable = Cast<UDataTable>(asset);
   return dataTable && dataTable->GetRowStruct()->IsChildOf(FTATMinimalQuestInfo::StaticStruct());
}

EDataValidationResult UDataTableValidator_Quest::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   const UDataTable* dataTable = CastChecked<UDataTable>(asset);

   auto reportError = [this, asset](const FText& message) {
         AssetFails(asset, message);
      };

   dataTable->ForeachRow<FTATMinimalQuestInfo>(TEXT("Validator"), [reportError](const FName& key, const FTATMinimalQuestInfo& quest)
      {
         const FGameplayTag questTag = quest.GetQuestTag();
         if (questTag.GetTagName() != key)
         {
            reportError(FText::FormatOrdered(INVTEXT("Quest row name ({0}) and tag ({1}) do not match, please make them the same"),
               FText::FromString(key.ToString()), FText::FromString(questTag.GetTagName().ToString())));
         }

         auto reportScoped = [&reportError, key](const FText& message) {
            reportError(FText::FormatOrdered(INVTEXT("[{0}] {1}"), FText::FromString(key.ToString()), message));
         };

         quest.Validate(reportScoped);
      });

   if (!IsValidationStateSet())
   {
      AssetPasses(asset);
   }
   return GetValidationResult();
}
