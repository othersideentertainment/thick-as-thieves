// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueInfo)

namespace TATClueInfoHelpers
{
   FString MakePreviewText(const FText& text, int32 maxLength, TCHAR quoteChar)
   {
      FString previewText = text.ToString();
      static const FStringView ellipsis = TEXT("...");
      maxLength = FMath::Max(maxLength, ellipsis.Len() + 1);
      if (previewText.Len() > maxLength)
      {
         const int32 numCharsToChop = previewText.Len() - (maxLength + ellipsis.Len());
         previewText.LeftChopInline(numCharsToChop, EAllowShrinking::No);
         previewText.Append(ellipsis);
      }
      if (quoteChar != 0)
      {
         previewText.InsertAt(0, quoteChar);
         previewText.AppendChar(quoteChar);
      }
      static const TArray<TCHAR> escapeChars = { '\n', '\r', '\t' };
      return previewText.ReplaceCharWithEscapedChar(&escapeChars);
   }

   FString FormatDebugDescription(FStringView clueType, FStringView description)
   {
      return FString::Format(TEXT("{0}: {1}"), { clueType, description });
   }
}

FTATClueBucketKey FTATClueInfo::GetClueBucket() const
{
   unimplemented();
   return {};
}
