// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/DataTableValidator_VoiceStateInfo.h"

// tat
#include "Interactables/TATInteractPromptStyle.h"

// ue5
#include "Engine/DataTable.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DataTableValidator_VoiceStateInfo)


bool UDataTableValidator_VoiceStateInfo::CanValidateAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context) const
{
   // TODO: convert to data driven array if there are multiple BP structs to do this for
   static const FName kStructName("BS_Audio_VoiceStateInfoStruct");

   const UDataTable* dataTable = Cast<UDataTable>(asset);
   return dataTable && dataTable->GetRowStruct()->GetFName() == kStructName;
}

EDataValidationResult UDataTableValidator_VoiceStateInfo::ValidateLoadedAsset_Implementation(const FAssetData& assetData, UObject* asset, FDataValidationContext& context)
{
   const UDataTable* dataTable = CastChecked<UDataTable>(asset);

   auto reportError = [this, asset](const FText& message) {
         AssetFails(asset, message);
      };

   static const FName kTagPropertyName("Tag_32_D5F362E74DD6A60D7236DD9C2AF4457C");
   const FProperty* property = dataTable->GetRowStruct()->FindPropertyByName(kTagPropertyName);
   if (!ensure(property))
   {
      reportError(INVTEXT("Could not find tag property"));
      return EDataValidationResult::Invalid;
   }

   for (TMap<FName, uint8*>::TConstIterator rowMapIter(dataTable->GetRowMap().CreateConstIterator()); rowMapIter; ++rowMapIter)
   {
      const FName key = rowMapIter.Key();
      const FGameplayTag* tag = property->ContainerPtrToValuePtr<FGameplayTag>(rowMapIter.Value());
      if (tag->GetTagName() != key)
      {
         reportError(FText::FormatOrdered(INVTEXT("Row name ({0}) and tag ({1}) do not match, please make them the same"),
            FText::FromString(key.ToString()), FText::FromString(tag->GetTagName().ToString())));
      }
   }

   if (!IsValidationStateSet())
   {
      AssetPasses(asset);
   }
   return GetValidationResult();
}
