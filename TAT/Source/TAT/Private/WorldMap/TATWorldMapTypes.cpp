// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "WorldMap/TATWorldMapTypes.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWorldMapTypes)

#if WITH_EDITOR
EDataValidationResult UTATMapSpriteDataAsset::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);
   TArray<FGameplayTag> foundTags;
   for (const FTATMapSpriteEntry& mapSpriteEntry : SpriteTable)
   {
      // Entry with invalid tag
      FGameplayTag tag = mapSpriteEntry.Tag;
      if (!tag.IsValid())
      {
         context.AddError(FText::FromString(TEXT("Entry with invalid Tag detected!")));
      }
      else
      {
         // Track usage count for all discovered tags
         if (foundTags.Contains(tag))
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate entries found with Tag [%s]!")
            , *tag.ToString())));
         }
         else
         {
            foundTags.Add(tag);
         }
      }

      // No sprite assigned
      if (mapSpriteEntry.Sprite.IsNull())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("Entry with Tag [%s] with invalid Sprite!")
         , *tag.ToString())));
      }
   }

   return context.GetIssues().IsEmpty() ? result : EDataValidationResult::Invalid;
}
#endif // WITH_EDITOR
