// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Upgrades/UpgradeCurrencies/TATUpgradeCurrencyMetadata.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUpgradeCurrencyMetadata)

const FTATCurrencyMetadata& UTATUpgradeCurrencyMetadataAsset::GetUpgradeCurrencyMetadata(FGameplayTag currencyTag) const
{
   const FTATCurrencyMetadata* found = _upgradeCurrencies.FindByKey(currencyTag);
   if (!ensure(found))
   {
      static const FTATCurrencyMetadata kNotFound;
      return kNotFound;
   }

   return *found;
}

bool UTATUpgradeCurrencyMetadataAsset::BP_GetUpgradeCurrencyMetadata(FGameplayTag currencyTag, FTATCurrencyMetadata& currencyMetadata) const
{
   if (const FTATCurrencyMetadata* found = _upgradeCurrencies.FindByKey(currencyTag))
   {
      currencyMetadata = *found;
      return true;
   }
   currencyMetadata = FTATCurrencyMetadata{};
   return false;
}

#if WITH_EDITOR
EDataValidationResult UTATUpgradeCurrencyMetadataAsset::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult baseResult = Super::IsDataValid(context);

   if (MoneyMetadata.Category != ETATCurrencyCategory::Money)
   {
      context.AddError(INVTEXT("MoneyMetadata should have Category == Money"));
   }

   // Make sure all currency tags are unique in the array
   TSet<FGameplayTag> foundCurrencyTags;
   for (int32 i = 0; i < _upgradeCurrencies.Num(); i++)
   {
      const FTATCurrencyMetadata& item = _upgradeCurrencies[i];

      if (item.Category != ETATCurrencyCategory::UpgradeCurrency)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Upgrade currency with tag '%s' (at index %d) should have Category == UpgradeCurrency."),
            *item.Tag.ToString(), i)));
      }

      if (!item.Tag.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Upgrade currency at index %d does not have a valid tag"), i)));
      }
      else if (foundCurrencyTags.Contains(item.Tag))
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate currency tag '%s' at index %d"), *item.Tag.ToString(), i)));
      }

      foundCurrencyTags.Add(item.Tag);
   }

   return (context.GetNumWarnings() + context.GetNumErrors() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR
