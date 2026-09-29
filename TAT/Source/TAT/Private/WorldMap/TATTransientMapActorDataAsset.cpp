// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "WorldMap/TATTransientMapActorDataAsset.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTransientMapActorDataAsset)
DEFINE_LOG_CATEGORY_STATIC(LogTATTransientMapActorDataAsset, Log, All);

#if WITH_EDITOR
EDataValidationResult UTATTransientMapActorDataAsset::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult result = Super::IsDataValid(context);

   TSet<FGameplayTag> foundIdentifiers;
   for (const FTATTransientMapActorEntry& mapActorEntry : TransientMapActorEntries)
   {
      if (!mapActorEntry.Identifier.IsValid())
      {
         context.AddError(FText::FromString(TEXT("Entry with invalid Identifier found!")));
      }
      bool alreadyPresent = false;
      foundIdentifiers.Add(mapActorEntry.Identifier, &alreadyPresent);

      if (alreadyPresent)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Multiple entries found with Identifier = %s!"), *mapActorEntry.Identifier.ToString())));
      }
   }
   return context.GetIssues().IsEmpty() ? result : EDataValidationResult::Invalid;
}
#endif // WITH_EDITOR

const FTATMapRepresentationData* UTATTransientMapActorDataAsset::GetMapRepresentationData(FGameplayTag identifier) const
{
   if (!identifier.IsValid())
   {
      UE_LOG(LogTATTransientMapActorDataAsset, Error, TEXT("GetMapRepresentationData() called with invalid Identifier!"));
      return nullptr;
   }
   const FTATTransientMapActorEntry* mapActorEntry = TransientMapActorEntries.FindByKey(identifier);
   if (!mapActorEntry)
   {
      UE_LOG(LogTATTransientMapActorDataAsset, Error, TEXT("GetMapRepresentationData() | could not find entry with Identifier = %s!"), *identifier.ToString());
      return nullptr;
   }

   return &mapActorEntry->MapRepresentationData;
}
