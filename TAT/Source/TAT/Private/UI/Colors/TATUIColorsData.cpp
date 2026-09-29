// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/Colors/TATUIColorsData.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUIColorsData)

#if WITH_EDITOR
EDataValidationResult UTATUIColorsData::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);
   
   auto validateSemanticPaletteColorEntry = [&context](const FGameplayTag& entryTag, const FTATUISemanticPaletteColorEntry& semanticColorPaletteEntry)
   {
      if (semanticColorPaletteEntry.LightColor == FLinearColor::Transparent)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("FTATUISemanticPaletteColorEntry %s has unassigned LightColor! Please assign a valid color")
            , *entryTag.ToString())));
      }
      if (semanticColorPaletteEntry.DarkColor == FLinearColor::Transparent)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("FTATUISemanticPaletteColorEntry %s has unassigned DarkColor! Please assign a valid color")
            , *entryTag.ToString())));
      }
   };

   for (const auto& mapEntry : SemanticColorPalettes)
   {
      const FGameplayTag& entryTag = mapEntry.Key;
      const FTATUISemanticPaletteColorEntry& colorPaletteEntry = mapEntry.Value;

      validateSemanticPaletteColorEntry(entryTag, colorPaletteEntry);
   }

   return context.GetNumErrors() + context.GetNumWarnings() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR
