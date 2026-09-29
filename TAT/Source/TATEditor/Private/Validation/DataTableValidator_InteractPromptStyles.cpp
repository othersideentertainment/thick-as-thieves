// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/DataTableValidator_InteractPromptStyles.h"

// tat
#include "Interactables/TATInteractPromptStyle.h"

// ue5
#include "Engine/DataTable.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DataTableValidator_InteractPromptStyles)


bool UDataTableValidator_InteractPromptStyles::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   const UDataTable* dataTable = Cast<UDataTable>(asset);
   return dataTable && dataTable->GetRowStruct() == FTATInteractPromptStyle::StaticStruct();
}

EDataValidationResult UDataTableValidator_InteractPromptStyles::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   const UDataTable* dataTable = CastChecked<UDataTable>(asset);

   auto reportError = [this, asset](const FText& message) {
         AssetFails(asset, message);
      };

   dataTable->ForeachRow<FTATInteractPromptStyle>(TEXT("Validator"), [reportError](const FName& key, const FTATInteractPromptStyle& interactPromptStyle)
      {
         if (interactPromptStyle.InteractActionTag.GetTagName() != key)
         {
            reportError(FText::FormatOrdered(INVTEXT("Interact Prompt Data row name ({0}) and tag ({1}) do not match, please make them the same"),
               FText::FromString(key.ToString()), FText::FromString(interactPromptStyle.InteractActionTag.GetTagName().ToString())));
         }

         auto reportScoped = [&reportError, key](const FText& message) {
            reportError(FText::FormatOrdered(INVTEXT("[{0}] {1}"), FText::FromString(key.ToString()), message));
         };

         if (!interactPromptStyle.InteractActionTag.IsValid())
         {
            reportScoped(INVTEXT("Interact Prompt Data has no key"));
         }
      });

   if (!IsValidationStateSet())
   {
      AssetPasses(asset);
   }
   return GetValidationResult();
}
